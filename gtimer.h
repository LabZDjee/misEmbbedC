#ifndef __GTIMER_DEFINED__
#define __GTIMER_DEFINED__

/**
 * \defgroup gTimer generic software timer
 * \{
 * This module is implements soft timers: function \c gtimerOnTick should be called when a hard timer tick has elapsed.
 * It is based on 32-bit counts and in order for conversion functions to work properly, it assumes associated timer tick to be one
 * millisecond or more.\n
 * Functions in this module allows to use a certain number of independent timers:\n
 * - defined by their zero based id's
 * - quantity defined by \c _N_GTIMERS (timer id's are always less than this value)
 *
 * For each of them, timer count can be started or stop, a timeout condition can be tested against a duration.
 * Timer can stop or automatically restart after this given duration.
 * It's also possible to associate a call-back function (of type #gtimerCallbackPtr)which is called when this duration has elapsed\n
 * None of those functions are thread safe and interrupt safe: so #gtimerOnTick as it is should not be called within
 * an interrupt but in main loop. However #gtimerOnTick will reset a flag #bTimerInterruptFired which
 * should be set by the interrupt. So #gtimerOnTick should be called when #bTimerInterruptFired is true\n
 * This module will work as long as \c _N_GTIMERS is defined. Also, \c GTIMER_TICK_MS should be defined: it instructs the
 * module how many milliseconds a tick represents. Considering the minimum value for this tick and the count word width, a minimum
 * delay of 49 days can be measured
 * \note management of callbacks take some more RAM in internal arrays. To avoid using memory for not needed functionalities,
 * \c GTIMER_IMPLEMENTS_CALLBACK should be defined in order to have this callback mechanism implemented
 * \warning For efficiency purpose, zero-based id of timers given as parameters of most of the functions
 * of this module are not controlled to be less than _N_GTIMERS. This is not safe and imposes external control or double check they cannot
 * be higher than expected
 * \file gtimer.h
 * \brief header of the gTimer module
 * \author Gerard Gauthier
 * \date 2015/01
 */

/****************************************************************************/
/*                                                                          */
/*                  MODULE DEPENDENCY                                       */
/*                                                                          */
/****************************************************************************/

#include "terms.h" /* where boolean, dword, word, byte... should be defined as types */
#include "system.h" /* where GTIMER_TICK_MS and _N_GTIMERS should be defined (also GTIMER_IMPLEMENTS_CALLBACK) */

/****************************************************************************/
/*                                                                          */
/*                  DEFINITIONS AND MACROS                                  */
/*                                                                          */
/****************************************************************************/

/* basic macros to transform ticks to time and time to ticks */
/** Timer tick to milliseconds, conversion macro */
#define GTIMER_TICK_TO_MS(ticks) ((dword)GTIMER_TICK_MS*(ticks))
/** Timer tick to seconds, conversion macro */
#define GTIMER_TICK_TO_S(ticks)  (((dword)GTIMER_TICK_MS*(ticks)+500)/1000)
/** Timer tick to minutes, conversion macro */
#define GTIMER_TICK_TO_MN(ticks) ((dword)GTIMER_TICK_MS*(ticks)+60*500)/60/1000)
/** Timer tick to hours, conversion macro */
#define GTIMER_TICK_TO_HRS(ticks) ((dword)GTIMER_TICK_MS*(ticks)+60*60*500))/60/60/1000)

/** milliseconds to timer ticks, conversion macro */
#define GTIMER_MS_TO_TICKS(ms) ((dword)(ms)/GTIMER_TICK_MS)
/** seconds to timer ticks, conversion macro */
#define GTIMER_S_TO_TICKS(s) ((dword)(s)*1000/GTIMER_TICK_MS)
/** minutes to timer ticks, conversion macro */
#define GTIMER_MN_TO_TICKS(m) ((dword)(m)*60*1000/GTIMER_TICK_MS)
/** hours to timer ticks, conversion macro */
#define GTIMER_HR_TO_TICKS(h) ((dword)(h)*60*60/GTIMER_TICK_MS)

/***************************/
/* Compiler Error Handler  */
/***************************/
#ifndef _N_GTIMERS
  #error _N_GTIMERS is not defined
#endif
#ifndef GTIMER_TICK_MS /* number of milliseconds a timer tick represents */
  #error GTIMER_TICK_MS is not defined
#endif

/****************************************************************************/
/*                                                                          */
/*                  TYPE DEFINITIONS                                        */
/*                                                                          */
/****************************************************************************/

#ifdef GTIMER_IMPLEMENTS_CALLBACK
/** \brief **callback mechanism: function automatically called on timeout**

 This function takes a user parameter in addition to the timer id and returns
 a value which may be used by user provided a non NULL pointer to return value
 is defined
 \see \c gtimerSetCallback
*/
typedef dword (*gtimerCallbackPtr)(byte id, dword value);
#endif

/****************************************************************************/
/*                                                                          */
/*                  EXPORTED GLOBAL VARIABLES                               */
/*                                                                          */
/****************************************************************************/
/**
  This flag should be set to true by a hard timer implementation, normally through an interrupt\n
  It should be tested as true before calling #gtimerOnTick
 */
volatile boolean bTimerInterruptFired; // true when the interrupt triggered and further process is needed

/****************************************************************************/
/*                                                                          */
/*                  PROTOTYPES OF EXPORTED FUNCTIONS                        */
/*                                                                          */
/****************************************************************************/

/** \brief **timer pulse**
 *
 * This routine should be called upon each tick, actually every time flag #bTimerInterruptFired
 * turns true
 * \note #bTimerInterruptFired is set to \c FALSE by this function
 */
void gtimerOnTick(void);

/** \brief **module initializer**
 *
 * Initializes all resources in this module
 * \warning This function must be called
 * prior to any other function of this module being called
 */
void gtimerInitModule(void);

/**\brief **request a timer out of the pool of available timers**
 *
 * This function requests one of the available resources. If a resource
 * is available, the resource id will be returned. If there are no more
 * resources available, \c _N_GTIMERS will be returned to indicate the error
 * situation. Other errors also return \c _N_GTIMERS
 * \return zero-based id of the timer, or \c _N_GTIMERS in case of error
 */
byte gtimerRequest(void);

/** \brief **reserves a timer with a given timer id**
 *
 * This function tries to reserve a specific resource id. If the resource
 * is available, the same resource id will be returned. If it was already
 * in use, \c _N_GTIMERS will be returned to indicate the error situation.
 * Other errors also return \c _N_GTIMERS
 * \return given parameter if ok, or \c _N_GTIMERS in case of error
 */
byte gtimerReserve(byte id /**< zero-based timer id */);

/** \brief **release timer id**
 *
 * This function releases the specified resource id. The resource is
 * set back to the initial state. Errors return \c _N_GTIMERS
 * \return given parameter if ok, or \c _N_GTIMERS in case of error
 */
byte gtimerRelease (byte id /**< zero-based timer id */);

/** \brief **starts a given timer to run**
 *
 * The timer stops after \c ticks *times* \c GTIMER_TICK_MS
 * Use the \ref gtimerTO timeout function to check whether or not the timer has elapsed.
 * If \c bAuto is \c TRUE, timer constantly runs, reloading its value as long it
 * has reached its timeout
 */
void gtimerInitAndStart (byte id /**< zero-based timer id */,
                         dword ticks /**< number of timer ticks before time-out */,
                         boolean bAuto /**< auto-reload flag */);

/** \brief **restarts the cycle initiated by #gtimerInitAndStart**
 *
 * It can be called any time during a cycle (before or after timeout condition has occurred)
 */
void gtimerRestart (byte id /**< zero-based timer id */);


/** \brief **stops timer counting**
 *
 * This function stops the given timer, leaving the current count value
 * intact The timer can be resumed for the remaining time by calling
 * #gtimerRestart
 */
void gtimerFreeze (byte id /**< zero-based timer id */);

/** \brief **resumes counting**
 *
 * This function restarts the timer after #gtimerFreeze for the time remaining
 */
void gtimerResume (byte id /**< zero-based timer id */);

/** \brief **rushes to time-out condition**
 *
 * This function ensures a timeout condition will occur within one timer tick
 */
void gtimerFastForward (byte id /**< zero-based timer id */);


/** \brief **is timer running?**
 *
 * \return \c TRUE if timer is running
 * \note When *not* in auto-mode and timer has reached its timeout, timer is
 *       considered *no longer* running and then this will return FALSE
 */
boolean gtimerRunning (byte id /**< zero-based timer id */);

/** \brief **has timer reached a time-out condition?**
 *
 * Informs whether timer count cycle has elapsed
 * \return \c TRUE if the time which was passed by the #gtimerInitAndStart function has
 * elapsed, and \c FALSE otherwise
 * \note when in auto mode and timeout has elapsed, this function will only
 * returned \c TRUE one time (per cycle): next calls to it within the next period
 * will return \c FALSE
 */
boolean gtimerTO (byte id /**< zero-based timer id */);

/** \brief **remaining time before time-out**
 *
 * \return the number of timer ticks to go before timeout will become \c TRUE
 * \note in auto mode, returns the number of ticks to finish the current cycle
 */
dword gtimerGetTimeToGo (byte id /**< zero-based timer id */);

#ifdef GTIMER_IMPLEMENTS_CALLBACK

/** \brief **attach call-back function**
 *
 * Set:
 * - timeout call-back function,
 * - its \p inValue, and
 * - a pointer to its \c out returned value.
 *   This pointer can be \c NULL and in this case the returned value is not passed
 */
void gtimerSetCallback (byte id /**< zero-based timer id */,
                        gtimerCallbackPtr pCallback /**< call back function */,
                        dword inValue /**< initial input value for the call-back function */,
                        dword* pOutValue/**< reference to out variable call-back can update */);

/** \brief **set new call-back input value**
 */
void gtimerSetCallbackInput (byte id /**< zero-based timer id */,
                        dword inValue /**< updated input value for the call-back function */);

/** \brief **detach call-back function**
 */
void gtimerClearCallback (byte id /**< zero-based timer id */);

/*
 * Disable any call-back previously defined
 */
#endif /* GTIMER_IMPLEMENTS_CALLBACK */

/**
 * \}
 */

#endif /* __GTIMER_DEFINED__ */
