/*

GCAU (Generic Control and Alarm Unit) C header file
File name: timer.h
Target: regulation co-processor
Purpose: Manager of soft timers
 Those timers are defined by their numbers _N_GTIMERS and GTIMER_TICK_MS, the number of milliseconds
 a tick will last
 Management is completely 'soft': function 'gtimerOnTick' should be called when every tick has elapsed
 Caveats: for efficiency purpose, zero-based id of timers given as parameters of most of the functions
          of this module are not controlled to be less than _N_GTIMERS. This is not safe
*/

#ifndef __GTIMER_DEFINED__
#define __GTIMER_DEFINED__

/****************************************************************************/
/*                                                                          */
/*                  MODULE DEPENDENCY                                       */
/*                                                                          */
/****************************************************************************/

#include "typedef.h" // where boolean, dword, word, byte... should be defined as types
#include "hal.h" // where GTIMER_TICK_MS and _N_GTIMERS should be defined (also GTIMER_IMPLEMENTS_CALLBACK)

/****************************************************************************/
/*                                                                          */
/*                  DEFINITIONS AND MACROS                                  */
/*                                                                          */
/****************************************************************************/

/* basic macros to transform ticks to time and time to ticks */
#define GTIMER_TICK_TO_MS(ticks) ((dword)GTIMER_TICK_MS*(ticks))
#define GTIMER_TICK_TO_S(ticks)  (((dword)GTIMER_TICK_MS*(ticks)+500)/1000)
#define GTIMER_TICK_TO_MN(ticks) ((dword)GTIMER_TICK_MS*(ticks)+60*500)/60/1000)
#define GTIMER_TICK_TO_HRS(ticks) ((dword)GTIMER_TICK_MS*(ticks)+60*60*500))/60/60/1000)

#define GTIMER_MS_TO_TICKS(ms) ((dword)(ms)/GTIMER_TICK_MS)
#define GTIMER_S_TO_TICKS(s) ((dword)(s)*1000/GTIMER_TICK_MS)
#define GTIMER_MN_TO_TICKS(m) ((dword)(m)*60*1000/GTIMER_TICK_MS)
#define GTIMER_HR_TO_TICKS(h) ((dword)(h)*60*60/GTIMER_TICK_MS)

/***************************/
/* Compiler Error Handler  */
/***************************/
#ifndef _N_GTIMERS
  #error _N_GTIMERS is not defined.
#endif
#ifndef GTIMER_TICK_MS /* number of milliseconds a timer tick represents */
  #error GTIMER_TICK_MS is not defined.
#endif


/****************************************************************************/
/*                                                                          */
/*                  TYPE DEFINITIONS                                        */
/*                                                                          */
/****************************************************************************/

#ifdef GTIMER_IMPLEMENTS_CALLBACK
/*
 callback mechanism: function automatically called on timeout
 this function takes a user parameter in addition to the timer id and returns
 a value which may be used by user provided a non NULL pointer to return value
 is defined
*/
typedef dword (*gtimerCallbackPtr)(byte id, dword value);
#endif
/****************************************************************************/
/*                                                                          */
/*                  EXPORTED GLOBAL VARIABLES                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*                  PROTOTYPES OF EXPORTED FUNCTIONS                        */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
void gtimerOnTick (void);
/****************************************************************************/
/* 
 * this one should be called upon each tick
 */

/****************************************************************************/
void gtimerInitModule (void);
/****************************************************************************/
/*
 * Initializes all resources in this module. This function must be called
 * before any other functions of this module are used
 */

/****************************************************************************/
byte gtimerRequest (void);
/****************************************************************************/
/*
 * This function requests one of the available resources. If a resource
 * is available, the resource id will be returned. If there are no more
 * resources available, _N_GTIMERS will be returned to indicate the error
 * situation. Other errors also return _N_GTIMERS
 */

/****************************************************************************/
byte gtimerReserve (byte id);
/****************************************************************************/
/*
 * This function tries to reserve a specific resource id. If the resource
 * is available, the same resource id will be returned. If it was already
 * in use, _N_GTIMERS will be returned to indicate the error situation.
 * Other errors also return _N_GTIMERS
 */
 
/****************************************************************************/
byte gtimerRelease (byte id);
/****************************************************************************/
/*
 * This function releases the specified resource id. The resource is
 * set back to the initial state. Errors return _N_GTIMERS
 */

/****************************************************************************/
void gtimerInitAndStart (byte id, dword ticks, boolean bAuto);
/****************************************************************************/
/*
 * This routine starts the given timer to run.
 * The timer stops after 'ticks X GTIMER_TICK_MS'
 * Use the gtimerTO timeout function to check whether or not the timer has elapsed.
 * if bAuto is TRUE, timer constantly runs, reloading its value as long it
 * has reached its timeout
 */

/****************************************************************************/
void gtimerRestart (byte id);
/****************************************************************************/
/*
 * This routine restarts the cycle initiated by gtimerInitAndStart
 * It can be called any time during a cycle (before or after timeout condition has occurred)
 */

/****************************************************************************/
void gtimerFreeze (byte id);
/****************************************************************************/
/*
 * This function stops the given timer, leaving the current count value
 * intact The timer can be resumed for the remaining time by calling
 * RestartTimer
 */

/****************************************************************************/
void gtimerResume (byte id);
/****************************************************************************/
/*
 * This function restarts the timer after gtimerFreeze for the time remaining
 */

/****************************************************************************/
void gtimerFastForward (byte id);
/****************************************************************************/
/*
 * This function ensures a timeout condition will occur within one tick
 */

/****************************************************************************/
boolean gtimerRunning (byte id);
/****************************************************************************/
/*
 * Returns TRUE if the Timer is running
 * Note: when not in auto-mode and timer has reached its timeout, Timer is
*        no longer running and then this will return FALSE
 */

/****************************************************************************/
boolean gtimerTO (byte id);
/****************************************************************************/
/*
 * Returns TRUE if the time which was passed by the gtimerInitAndStart function has
 * elapsed and FALSE otherwise
 * Note: when in auto mode and timeout has elapsed, this function will only 
 * returned true one time (per cycle): next calls to it when the next period
 * is not timed out will return FALSE
 */

/****************************************************************************/
dword gtimerGetTimeToGo (byte id);
/****************************************************************************/
/*
 * Returns the number of ticks to go before timeout will become TRUE
 */

#ifdef GTIMER_IMPLEMENTS_CALLBACK
/****************************************************************************/
void gtimerSetCallback (byte id, gtimerCallbackPtr pCallback, dword inValue, dword* pOutValue);
/****************************************************************************/
/*
 * Set timeout call back function, its 'in' value and a pointer to its 'out' returned value
 *  this pointer can be NULL and in this case the returned value is not communicated
 */

/****************************************************************************/
void gtimerClearCallback (byte id);
/****************************************************************************/
/*
 * Disable any callback previously defined
 */


#endif
#endif
