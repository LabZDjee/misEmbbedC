#include "swUart.h"

#include "gtimer.h"

#define QTY_OF_SENDERS (1)
#define QTY_OF_RECEIVERS (1)

#define MIN_BITS_SERIALIZED (3)
#define MAX_BITS_SERIALIZED (10)

/* for space constraints environments can be set to byte.
   in this case characters more than 8-bit wide will be trimmed to a byte */
#define RCPT_FIFO_TYPE          word /* byte or word */
/* size of reception FIFO as defined in number of bits:
 * - 0 for size 1 (2 to the power of 0),
 * - 1 for size 2,
 * - 2 for size 4, ...
 * - up to 8 for size 256 (maximum value)
 */
#define RCPT_FIFO_SIZE_IN_BITS (4) /* so size is 2^RCPT_FIFO_SIZE_IN_BITS */
#define RCPT_FIFO_SIZE (1 << RCPT_FIFO_SIZE_IN_BITS) /* in case we need the associated size */
#define RCPT_FIFO_SIZE_MASK (RCPT_FIFO_SIZE - 1) /* divide mask using the '&' operator */

/* machine states for transmission */
typedef enum { swUTIdle, swUTEdge, swUTStart, swUTBit, swUTParity, swUTStop1, swUTStop2 } swUartTxStateE;
/* machine states for reception */
typedef enum { swURIdle, swURStart, swURSBit, swURParity, swURStop1, swURStop2 } swUartRxStateE;

/*
 * common part of state machine context
 */
typedef struct
{
 word serialChar;    /* char/word being serialized */
 byte timerId;    /* id of gTimer taking care of delays */
 byte bitPos : 4;  /* what bit is presently processed */
 byte nbOfBitsSet : 4;  /* for parity calculation */
} swUartCommonPartOfStateMachineT;

/*
 * state machine for transmission
 */
typedef struct
{
 swUartTxStateE state;    /* machine state */
 swUartHwSetTxFct txFct;    /* i/o function for setting Tx line state (mark or space) */
 swUartCommonPartOfStateMachineT c;    /* common part between Tx and Rx state machines */
 byte bInProgress : 1;  /* activity going on */
} swUartTxStateMachineT;

/*
 * state machine for reception
 */
typedef struct
{
 swUartRxStateE state;    /* machine state */
 byte error;    /* reception error */
 swUartHwGetRxFct rxFct;    /* i/o function for reading Rx line state (mark or space) */
 swUartCommonPartOfStateMachineT c;    /* common part between Tx and Rx state machines */
 RCPT_FIFO_TYPE rxFifo[1 << RCPT_FIFO_SIZE_IN_BITS];  /* FIFO queue to store incoming characters */
 byte rxFifoReadIndex;    /* location where an available character can be read */
 byte rxFifoWriteIndex;    /* location where an available character can be written  */
 byte bInProgress : 1;  /* activity going on */
 byte rxFifoEmpty : 1;  /* FIFO queue is empty - read and write index equals and queue is not full */
 byte dontStore : 1;  /* We got an error: don't store serialized value */
 byte scanPosition : 2;  /* sub state when scanning Rx state multiple times */
 byte scanValues : 2;  /* where to store scanned values */
} swUartRxStateMachineT;

/*
 * transmission data structure
 */
typedef struct _swUartTxStruct
{
 const swUartConfigurationT* pCfg;    /* configuration */
 swUartTxStateMachineT sm;    /* state machine */
} swUartTxStruct;

/*
 * reception data structure
 */
typedef struct _swUartRxStruct
{
 const swUartConfigurationT* pCfg;    /* configuration */
 swUartRxStateMachineT sm;    /* state machine */
} swUartRxStruct;

/* arrays of transmitter and receiver data structures */
static swUartTxStruct _sendSArray[QTY_OF_SENDERS];
static swUartRxStruct _receiveSArray[QTY_OF_RECEIVERS];

/************************************************************
 * transmission section
 ************************************************************/

static dword swUartSendCallBack(UNUSED_FCT_P byte id, dword value)
{
 swUartTxStruct* pSendStruct = (swUartTxStruct*)value;
 swUartTxStateMachineT* pSM = &pSendStruct->sm;
 const swUartConfigurationT* pCfg = pSendStruct->pCfg;
 /* in this state machine, when we reach a state it means this state is done (pCfg->bitWidth
    has elapsed) and we should act on next
    this means we position the line state when new state is assigned */
 switch (pSM->state)
  {
   default:
   case swUTIdle:  /* should never happen, as send routine which start timer set state to swUTStart */
    return 0;
   case swUTEdge:
    pSM->txFct(swUSpace_Low);
    pSM->state = swUTStart;
    goto reloadTimerAndReturn;
   case swUTStart:
    pSM->state = swUTBit;   /* simply next stage, initializing bit counters */
    pSM->c.bitPos = pSM->c.nbOfBitsSet = 0;
    goto processBit;     /* can proceed to next stage now */
   case swUTParity:
    goto processStop1;     /* directly position line for first data bit (b0) */
   case swUTBit:
    if (pSM->c.bitPos >= pCfg->nbBits)  /* test whether we reached all the necessary bits for a character? */
     {
      switch (pCfg->parity)      /* necessary to position a parity? */
       {
        case swUNoParity:     /* no: let's start stop bit */
        processStop1:
         pSM->state = swUTStop1;
         pSM->txFct(swUMark_High);
         goto reloadTimerAndReturn;
        case swUEvenParity:
         /* even parity: parity bit set to high if number of high bits in data is odd */
         pSM->txFct(pSM->c.nbOfBitsSet & 1 ? swUMark_High : swUSpace_Low);
         goto endOfParityProcessing;
        case swUOddParity:
         pSM->txFct(pSM->c.nbOfBitsSet & 1 ? swUSpace_Low : swUMark_High);
        endOfParityProcessing:
         pSM->state = swUTParity;      /* to notify parity is done on next callback */
         goto reloadTimerAndReturn;
       }
     }
   processBit:
    if (pSM->c.serialChar & 1)
     {
      pSM->txFct(swUMark_High);
      pSM->c.nbOfBitsSet++;       /* for parity control (if needed) */
     }
    else
     {
      pSM->txFct(swUSpace_Low);
     }
    pSM->c.serialChar >>= 1;   /* Low Significant Bit first, so shift right 1 bit */
    pSM->c.bitPos++;
    goto reloadTimerAndReturn;
   case swUTStop1:
    if (pCfg->stop == swU2Stop)
     {
      pSM->state = swUTStop2;     /* 2-stop: stretch stop one bit */
      goto reloadTimerAndReturn;
     }
   /* only one stop bit: NO BREAK on purpose */
   case swUTStop2:
    pSM->bInProgress = FALSE;   /* end of character transmission */
    pSM->state = swUTIdle;
    return 0;
  }
reloadTimerAndReturn:
 gtimerRestart(pSM->c.timerId);    /* takes advantage the timer delay never changes */
 return 0;
}

boolean swUartSendInit(byte swUartTxId, const swUartConfigurationT* pCfg, byte timerId, swUartHwSetTxFct txFct)
{
 swUartTxStateMachineT* pSM = &_sendSArray[swUartTxId].sm;
 /* gate keeper */
 if (swUartTxId >= QTY_OF_SENDERS || pCfg == NULL || txFct == NULL)
  {
   return FALSE;
  }
 if (pCfg->bitWidth < 2)
  {
   return FALSE;       /* cannot handle less than 2 gTimer ticks */
  }
 /* makes sure number of bits sits within boundaries */
 if (pCfg->nbBits < MIN_BITS_SERIALIZED || pCfg->nbBits > MAX_BITS_SERIALIZED)
  {
   return FALSE;
  }
 _sendSArray[swUartTxId].pCfg = pCfg;  /* hooks configuration */
 pSM->c.timerId = timerId;
 pSM->txFct = txFct;
 pSM->bInProgress = FALSE;
 pSM->state = swURIdle;
 txFct(swUMark_High);    /* sets line in mark, idle state */
 gtimerFreeze(timerId);
 /* installs callback with transmit data array reference as an immutable parameter */
 gtimerSetCallback(timerId, swUartSendCallBack, (long)_sendSArray + swUartTxId, NULL);
 return TRUE;
}

boolean swUartSendIsBusy(byte swUartTxId)
{
 if (swUartTxId >= QTY_OF_SENDERS)
  {
   return TRUE;
  }
 return _sendSArray[swUartTxId].sm.bInProgress;
}

boolean swUartSendChar(byte swUartTxId, word ch)
{
 swUartTxStateMachineT* pSM = &_sendSArray[swUartTxId].sm;
 /* this should reasonably fail if transmission data has never been initialized */
 if (swUartTxId >= QTY_OF_SENDERS || _sendSArray[swUartTxId].pCfg == NULL || pSM->bInProgress)
  {
   return FALSE;
  }
 /* primes state machine */
 pSM->bInProgress = TRUE;
 pSM->c.serialChar = ch;
 pSM->state = swUTEdge;
 /* sets the timer at one bit delay, in manual mode */
 gtimerInitAndStart(pSM->c.timerId, (dword)_sendSArray[swUartTxId].pCfg->bitWidth, FALSE);
 return TRUE;
}

boolean swUartSendData(byte swUartTxId, const void* data, word dataSize, word* pIndex)
{
 const word* pW;
 const byte* pB;
 if (swUartTxId >= QTY_OF_SENDERS || _sendSArray[swUartTxId].pCfg == NULL || pIndex == NULL)
  {
   return TRUE;    /* don't pass gate-keeper: reports everything as done */
  }
 if (swUartSendIsBusy(swUartTxId) == FALSE) /* can proceed with next char */
  {
   if (*pIndex == dataSize)
    {
     return TRUE;          /* no next char: reports as done */
    }
   if (_sendSArray[swUartTxId].pCfg->nbBits > 8)    /* does not fit in 8 bits  */
    {
     pW = (word*)data;       /* consider data buffer as composed of words */
     swUartSendChar(swUartTxId, pW[(*pIndex)++]);
    }
   else       /* fits within a byte */
    {
     pB = (byte*)data;       /* considers data buffer accordingly */
     swUartSendChar(swUartTxId, pB[(*pIndex)++]);
    }
  }
 return FALSE;
}

/************************************************************
 * reception section
 ************************************************************/

static dword swUartReceiveCallBack(UNUSED_FCT_P byte id, dword value)
{
 swUartRxStruct* pReceiveStruct = (swUartRxStruct*)value;
 swUartRxStateMachineT* pSM = &pReceiveStruct->sm;
 const swUartConfigurationT* pCfg = pReceiveStruct->pCfg;
 swUartDataStateE lineLevel;    /* stores bit level to consider */
 word timerReloadValue;

 if (pSM->state == swURIdle) /* should never happen */
  {
   return 0;
  }
 if (pCfg->bTripleScan) /* noise canceler: scans three times 1/4 bit then waits for 1/2 bit */
  {
   if (pSM->rxFct() == swUMark_High)
    {
     pSM->scanValues++;
    }
   pSM->scanPosition++;
   if (pSM->scanPosition < 3)   /* scan of one bit not finished */
    {
     timerReloadValue = pCfg->bitWidth >> 2;
     goto reloadTimerAndReturn;
    }
   /* scan finished */
   timerReloadValue = pCfg->bitWidth >> 1;
   /* prepared to scan next ones */
   pSM->scanValues = 0;
   pSM->scanPosition = 0;
   /* level set by majority report, will process it now */
   lineLevel = pSM->scanValues > 1 ?  swUMark_High : swUSpace_Low;
  }
 else    /* no noise cancellation */
  {
   lineLevel = pSM->rxFct();    /* just scans once */
   timerReloadValue = pCfg->bitWidth;    /* next scan after one-bit delay */
  }
 /* start of state machine, lineLevel and timerReloadValue defined */
 switch (pSM->state)
  {
   default:
   case swURIdle:  /* should never happen because timer is stopped when so */
    return 0;
   case swURStart:
    if (lineLevel != swUSpace_Low)  /* start bit has to be a a low (0), so a space */
     {
      pSM->error |= 1 << swUFramingError;   /* didn't get the expected start: error */
      goto stopSM;       /* and abort reception */
     }
    pSM->state = swURSBit;
    pSM->dontStore = FALSE;   /* no reason we should shun store character, if no error  */
    pSM->c.bitPos = 0;
    pSM->c.nbOfBitsSet = 0;   /* for parity */
    pSM->c.serialChar = 0;   /* char will be stored here */
    goto reloadTimerAndReturn;
   case swURSBit:
    if (lineLevel == swUMark_High)
     {
      pSM->c.nbOfBitsSet++;       /* for parity */
      pSM->c.serialChar |= 1 << pSM->c.bitPos;   /* Least Significant Bit first */
     }
    if (++pSM->c.bitPos >= pCfg->nbBits) /* reached required number of bits then stop bit if no parity or parity check */
     {
      if (pCfg->parity == swUNoParity)
       {
        pSM->state = swURStop1;
       }
      else
       {
        pSM->state = swURParity;
       }
     }
    goto reloadTimerAndReturn;
   case swURParity:
    if (pSM->c.nbOfBitsSet & 1)  /* odd number of bits set */
     {
      /* if parity is even logic should be high (1), so line should be mark */
      if (lineLevel != ((pCfg->parity == swUEvenParity) ? swUMark_High : swUSpace_Low))
       {
        pSM->error |= 1 << swUParityError;
        pSM->dontStore = TRUE;
       }
     }
    else     /* even number of bits set */
     {
      /* if parity is even logic should be low (0), so line should be space */
      if (lineLevel != ((pCfg->parity == swUEvenParity) ? swUSpace_Low : swUMark_High))
       {
        pSM->error |= 1 << swUParityError;
        pSM->dontStore = TRUE;
       }
     }
    pSM->state = swURStop1;
    goto reloadTimerAndReturn;
   case swURStop1:
    if (lineLevel != swUMark_High)
     {
      pSM->error |= 1 << swUFramingError;
      pSM->dontStore = TRUE;
     }
    if (pCfg->stop == swU1Stop)
     {
      goto storeValueStage;
     }
    pSM->state = swURStop2;
    goto reloadTimerAndReturn;
   case swURStop2:
    if (pSM->rxFct() != swUMark_High)
     {
      pSM->error |= 1 << swUFramingError;
      pSM->dontStore = TRUE;
     }
   storeValueStage:
    if (pSM->dontStore == FALSE)
     {
      /* stores value in the FIFO */
      byte newIndex = (pSM->rxFifoWriteIndex + 1) & RCPT_FIFO_SIZE_MASK; /* useful to pre-calculate new write index in case of overrun */
      pSM->rxFifo[pSM->rxFifoWriteIndex] = (RCPT_FIFO_TYPE)pSM->c.serialChar;
      if (pSM->rxFifoEmpty == FALSE)
       {
        if (pSM->rxFifoWriteIndex == pSM->rxFifoReadIndex)      /* ouch, overrun */
         {
          pSM->error |= 1 << swUOverrunError;
          pSM->rxFifoReadIndex = newIndex;         /* one character lost */
         }
       }
      pSM->rxFifoWriteIndex = newIndex;
      pSM->rxFifoEmpty = FALSE;     /* never can be empty, obviously */
     }
   stopSM:
    pSM->bInProgress = FALSE;
    pSM->state = swURIdle;   /* timer is manual, so the state machine will stop here */
    return 0;
  }
reloadTimerAndReturn:
 /* timer is manual, so we reload it here */
 gtimerInitAndStart(pSM->c.timerId,
                    (dword)timerReloadValue,
                    FALSE);
 return 0;
}

boolean swUartReceiveInit(byte swUartRxId, const swUartConfigurationT* pCfg, byte timerId, swUartHwGetRxFct rxFct)
{
 swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
 if (swUartRxId >= QTY_OF_RECEIVERS || pCfg == NULL || rxFct == NULL)
  {
   return FALSE;
  }
 if (pCfg->bTripleScan) /* noise canceler */
  {
   if ((pCfg->bitWidth & (2 * 4 - 1)) != 0) /* x2 because of gTimer minimum delay */
    {
     return FALSE;         /* cannot handle 1/4 bit delay: error */
    }
  }
 else    /* no noise canceler */
  {
   if ((pCfg->bitWidth & (2 * 2 - 1)) != 0) /* x2 because of gTimer minimum delay */
    {
     return FALSE;          /* cannot handle 1/2 bit delay: error */
    }
  }
 /* makes sure number of bits sits within boundaries */
 if (pCfg->nbBits < MIN_BITS_SERIALIZED || pCfg->nbBits > MAX_BITS_SERIALIZED)
  {
   return FALSE;
  }
 _receiveSArray[swUartRxId].pCfg = pCfg;
 pSM->c.timerId = timerId;
 pSM->bInProgress = FALSE;
 /* error only reset automatically here */
 pSM->error = 0;
 pSM->rxFct = rxFct;
 pSM->rxFifoEmpty = TRUE;
 pSM->rxFifoReadIndex = pSM->rxFifoWriteIndex = 0;
 pSM->state = swURIdle;
 gtimerFreeze(timerId);
 /* installs callback with transmit data array reference as an immutable parameter */
 gtimerSetCallback(timerId, swUartReceiveCallBack, (long)_receiveSArray + swUartRxId, NULL);
 return TRUE;
}

boolean swUartReceiveScanForStart(byte swUartRxId)
{
 swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
 const swUartConfigurationT* pCfg = _receiveSArray[swUartRxId].pCfg;
 if (swUartRxId >= QTY_OF_RECEIVERS)
  {
   return FALSE;
  }
 if (pSM->bInProgress == FALSE && pSM->rxFct() == swUSpace_Low) /* detected a space on the line: Low (0) */
  {
   pSM->bInProgress = TRUE;
   pSM->state = swURStart;
   if (pCfg->bTripleScan)
    {
     pSM->scanValues = 0;   /* will store different samples scanned */
     pSM->scanPosition = 0;   /* will count them */
    }
   /* sets timer to scan next center bit with or 1/4 of bit duration in case of noise cancellation
      this will start the reception state machine */
   gtimerInitAndStart(pSM->c.timerId,
                      (dword)(pCfg->bitWidth >> (pCfg->bTripleScan ? 2 : 1)),
                      FALSE);
   return TRUE;       /* reports a reception under progress */
  }
 return FALSE;
}

word swUartPeekChar(byte swUartRxId)
{
 swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
 if (swUartRxId >= QTY_OF_RECEIVERS || pSM->rxFifoEmpty)
  {
   return 0xffff;
  }
 return (word)pSM->rxFifo[pSM->rxFifoReadIndex];
}

word swUartGetChar(byte swUartRxId)
{
 byte readIndex;
 swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
 if (swUartRxId >= QTY_OF_RECEIVERS || pSM->rxFifoEmpty)
  {
   return 0xffff;
  }
 readIndex = pSM->rxFifoReadIndex;  /* snaps index before increment it */
 pSM->rxFifoReadIndex = (pSM->rxFifoReadIndex + 1) & RCPT_FIFO_SIZE_MASK;
 if (pSM->rxFifoReadIndex == pSM->rxFifoWriteIndex) /* FIFO dried out */
  {
   pSM->rxFifoReadIndex = pSM->rxFifoWriteIndex = 0;
   pSM->rxFifoEmpty = TRUE;
  }
 return (word)pSM->rxFifo[readIndex];
}

byte swUartHowManyChars(byte swUartRxId)
{
 swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
 int ret;
 if (swUartRxId >= QTY_OF_RECEIVERS || pSM->rxFifoEmpty)
  {
   return 0;
  }
 /* we suppose write is ahead of read in the FIFO */
 ret = pSM->rxFifoWriteIndex - pSM->rxFifoReadIndex;
 if (ret <= 0) /* nope */
  {
   ret = (int)RCPT_FIFO_SIZE - ret;  /* so corrects by 'unfolding' the FIFO index */
  }
 return (byte)ret;
}

word swUartPeekNChar(byte swUartRxId, byte nth)
{
 if (nth < swUartHowManyChars(swUartRxId))
  {
   swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
   /* as nth fits a simple modulo operation can reach it: char at index (readIndex+nth) % RCPT_FIFO_SIZE  */
   return (word)pSM->rxFifo[(pSM->rxFifoReadIndex + nth) & RCPT_FIFO_SIZE_MASK];
  }
 return 0xffff;
}

void swUartFlushChars(byte swUartRxId)
{
 if (swUartRxId < QTY_OF_RECEIVERS)
  {
   swUartRxStateMachineT* pSM = &_receiveSArray[swUartRxId].sm;
   pSM->rxFifoReadIndex = pSM->rxFifoWriteIndex = 0;
   pSM->rxFifoEmpty = TRUE;
  }
}

byte swUartReceiveGetAndClearError(byte swUartRxId, boolean bClearError)
{
 byte error;
 if (swUartRxId >= QTY_OF_RECEIVERS)
  {
   return 0;
  }
 error = _receiveSArray[swUartRxId].sm.error;
 if (bClearError)
  {
   _receiveSArray[swUartRxId].sm.error = 0;
  }
 return error;
}
