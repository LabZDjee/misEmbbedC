#ifndef __SWUART_H_INCLUDED__
#define __SWUART_H_INCLUDED__
/**
 * \defgroup swUart generic software UART (Universal Asynchronous Reception Transmission)
 * \{
 * Purpose
 * =======
 * This module, \c swUart, is a working example of an asynchronous reception/transmission (Rx/Tx) implementation
 * completely software driven and associated with another software driven module: \c gTimer, \ref gTimer with
 * provision for callbacks\n
 * Both modules are generic. They are mainly intended to work in embedded contexts which are _not_ extremely
 * hard time constraint\n
 * Details
 * =======
 * Module \c gTimer run in the main loop\n
 * Software UART's run as state machines which are implemented as \c gTimer callbacks\n
 * This C code is portable (mainly written for and tested under \c GCC) can be tested on a PC box for evaluation
 * and ported on an embedded platform with available access of I/O line and a possibility to hook \c gTimer to a system
 * timer\n
 * Transmit and receive sections are clearly separated (so the optimizer should do its job and
 * eliminate what is not used)\n
 * More than one transmitter and one receiver can coexist: their respective maximum quantities are statically
 * defined by QTY_OF_SENDERS and QTY_OF_RECEIVERS constants which set sizes of internal arrays of management structures
 * in this module\n
 * Each transmitter is defined by:
 * - its configuration \ref swUartConfigurationT,
 * - a dedicated \ref gTimer, and
 * - an I/O function of type \ref swUartHwSetTxFct whose role is to set the transmission line
 *
 * Each receiver is defined by:
 * - its configuration \ref swUartConfigurationT,
 * - a dedicated \ref gTimer, and
 * - an I/O function of type \ref swUartHwGetRxFct whose role is to read the reception line
 * How to
 * ======
 *
 * \file swUart.h
 * \brief header of the swUart module
 * \author Gerard Gauthier
 * \date 2015/03
 */

#include "terms.h" /** defines common and intuitive integer types like byte, word, dword, also TRUE, NULL...  */

/** defines parity */
typedef enum _swUartParityE
 { swUEvenParity=2, swUOddParity=1, swUNoParity=0 } swUartParityE;
/** defines number of stop bits */
typedef enum _swUartStopBitsE
 { swU1Stop=1, swU2Stop=2 } swUartStopBitsE;
/** \brief defines space and mark line states
 *
 * - space: 0 / LOW  / 0V (TTL) / +5~15V (EIA-232)
 * - mark:  1 / HIGH / 5V (TTL) / -5~15V (EIA-232)
 */
typedef enum _swUartDataStateE
 { swUSpace_Low=0, swUMark_High=1 } swUartDataStateE;

/** reception errors */
typedef enum _swUartErrorBitDefinitionE
 { swUFramingError=0, swUParityError=1, swUOverrunError=2 } swUartErrorBitDefinitionE;

/** I/O function type for setting the Tx line */
typedef void (*swUartHwSetTxFct)(swUartDataStateE value);
/** I/O function type for reading the Rx line */
typedef swUartDataStateE (*swUartHwGetRxFct)(void);

/** \brief swUART configuration structure
 *
 * this structure is to be set to user values and will be passed to initializer that will keep
 * a reference on it
 * \note though theoretically, \p bitWith should be a \c dword, it is declared a word to save some space
 * in this structure and in the module management\n
 * the fact is it has been estimated extremely unlikely to deal with bit rates imposing this value to be
 * above 65,535
 * \warning
 *  - cannot be read-only as initializer *may* correct sick parameters, however immutable after initialization
 *  - \p bitWidth has to be _even_ in case of \p bTripleScan is \c FALSE and a multiple of _four_ in case
 *    \p bTripleScan is \c TRUE
 */
typedef struct _swUartConfigurationT
{
 word bitWidth; /**< duration of one bit, unit is timer reload value */
 swUartStopBitsE stop; /**< number of stop bits: 1 or 2 */
 swUartParityE parity; /**< type of parity: none, even, or odd */
 byte nbBits; /**< number of bits to serialize: 3 to 10 */
 boolean bTripleScan; /**< if each bit is scanned three times on reception (instead of one time) */
}swUartConfigurationT;

/************************************************************
 * transmission section
 ************************************************************/

/** \brief initializes a swUART sending state machine
 *
 * this initializer should be called before any call to send function with the same \c swUartTxId
 * \return TRUE if parameters are acceptable (no reference is NULL, \c swUartTxId is within bounds...)
 *
 */
boolean swUartSendInit(byte swUartTxId /**< swUART Tx index, zero-based */,
                       swUartConfigurationT* pConfig /**< reference to configuration parameters */,
                       byte timerId /**< index of the generic timer that will controls proper delays */,
                       swUartHwSetTxFct txFct /**< I/O function to set a space or a break position */);

/** \brief sends a character through a state machine of identifier \p swUartTxId
 *
 * \note \p ch is a word because more than 8 bits can be sent if configuration is such
 * \return \c TRUE if \p ch is being serialized, essentially if sending state machine is not busy and correctly initialized
 */
boolean swUartSendChar(byte swUartTxId /**< swUart Tx index, zero-based */, word ch /**< character to be serialized */);

/** \brief not available for transmit
 *
 * \return \c TRUE if transmission is busy and would not accept sending something
 */
boolean swUartSendIsBusy(byte swUartTxId /**< swUart Tx index, zero-based */);

/** \brief send a stream of data
 *
 * sends a stream of data of size dataSz\n
 * the caller must provide reference to a word which stores where we are in the sending: \p pIndex\n
 * the value at this reference address must be set to zero\n
 * \note what \p data references is an array of bytes if field \n nbBits in configuration structure
 * \ref swUartConfigurationT is less than 9 and it is an array of _words_ otherwise
 * \return \c TRUE when send streaming is completed
 */
boolean swUartSendData(byte swUartTxId /**< swUart Tx index, zero-based */,
                        const void* data /**< data to send, array of bytes or words depending on \c nbBits */,
                        word dataSz /**< number of data items to send */,
                        word* pIndex /**< reference to where we are in the sending */);

/************************************************************
 * reception section
 ************************************************************/

/** \brief initializes a swUART receiving state machine and logics
 *
 * this initializer should be called before any call to receive function with the same \p swUartRxId
 * \return TRUE if parameters are acceptable (no reference is NULL, \p swUartRxId is within bounds...)
 */
boolean swUartReceiveInit(byte swUartRxId /**< swUART Rx index, zero-based */,
                          swUartConfigurationT* pCfg /**< reference to configuration parameters */,
                          byte timerId /**< index of the generic timer that will controls proper delays */,
                          swUartHwGetRxFct rxFct /**< I/O function to sense a space or a break position */);

/** \brief test the reception line and engage the reception state machine when start condition sensed
 *
 * this function has to be called as often as possible to test whether a start condition on the reception line has occurred\n
 * if a start condition has occurred the reception state machine is primed\n
 * it can be safely called when a reception is already under way
 * \return \c TRUE iff a start condition has just been sensed
 */
boolean swUartReceiveScanForStart(byte swUartRxId /**< swUart Tx index, zero-based */);

/** \brief returns the least recently received character leaving the FIFO unaffected
 *
 * can be used to test if some character is ready to be read
 * \return most recent character or 0xffff if reception FIFO is empty (or if an error occurred)
 */
word swUartPeekChar(byte swUartRxId /**< swUART Rx index, zero-based */);

/** \brief returns the nth received character leaving the FIFO unaffected
 *
 * least recently received character is of rank zero
 * \return the targeted character or 0xffff if reception FIFO is empty (or if an error occurred)
 */
word swUartPeekNChar(byte swUartRxId /**< swUART Rx index, zero-based */, byte nth /**< zero-based rank of character */);

/** \brief number of characters available in the reception FIFO
 *
 * \return this number - zero returned not only when FIFO is empty but also in case of error
 */
byte swUartHowManyChars(byte swUartRxId /**< swUART Rx index, zero-based */);

/** \brief empties the reception FIFO
 */
void swUartFlushChars(byte swUartRxId /**< swUART Rx index, zero-based */);

/** \brief pulls available character from the reception FIFO
 *
 * this character is the least recently received
 * \return this character and 0xffff in case of error or if FIFO is empty
 */
word swUartGetChar(byte swUartRxId /**< swUART Rx index, zero-based */);

/** \brief return the error value and possibly clear it
 *
 * the error is a bit-field with bit set according to definitions stated in \b swUartErrorBitDefinitionE
 * \return value of the error before it is possibly cleared
 */
boolean swUartReceiveGetAndClearError(byte swUartRxId /**< swUart Tx index, zero-based */,
                                      boolean bClearError /**< \c TRUE, should the error be cleared */);

/**
 * \}
 */

#endif // __SWUART_H_INCLUDED__
