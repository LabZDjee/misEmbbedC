/*

File name: gtimer.c
Target: portable
Purpose: Manager of soft timers

*/

#define _TIMER_IMPLEMENTATION

/****************************************************************************/
/*                                                                          */
/*                  MODULE DEPENDENCY                                       */
/*                                                                          */
/****************************************************************************/

#include "gtimer.h"

/****************************************************************************/
/*                                                                          */
/*                  DEFINITIONS AND MACROS                                  */
/*                                                                          */
/***************************************************************************/

#define TIMER_COUNT_VALUE   ((GTIMER_TICK_MS)*(OSC_FREQUENCY)/(1000))

typedef struct
{
byte         req:1;                    // in use or not
byte         running:1;                // started or not
byte         timeout:1;                // reached a timeout
byte         bAuto:1;                  // auto-reload or not
dword        count;                    // periods of GTIMER_TICK_MS to run before time out
dword        count0;                   // reload value
#ifdef GTIMER_IMPLEMENTS_CALLBACK
gtimerCallbackPtr  pCallback;          // function to call when timer expires
dword        inValue;                  // its 'in' parameter
dword*       pOutValue;                // reference to its 'out' parameter
#endif
}
TimerType;

/****************************************************************************/
/*                                                                          */
/*                  PROTOTYPES OF NOT EXPORTED FUNCTIONS                    */
/*                                                                          */
/****************************************************************************/

static void init_Timer    (byte id);

/****************************************************************************/
/*                                                                          */
/*                  EXPORTED / IMPORTED GLOBAL VARIABLES                    */
/*                                                                          */
/****************************************************************************/

volatile boolean bTimerInterruptFired; // true when the interrupt triggered and further process is needed

/****************************************************************************/
/*                                                                          */
/*                  NOT EXPORTED GLOBAL VARIABLES                           */
/*                                                                          */
/****************************************************************************/

/* array of gTimers */
static TimerType Timer [_N_GTIMERS];

/****************************************************************************/
/*                                                                          */
/*                  EXPORTED FUNCTIONS                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
void gtimerInitModule (void)
/****************************************************************************/
{
 byte id;

 for (id = 0; id < _N_GTIMERS; id++)
  init_Timer (id);
}


/****************************************************************************/
byte gtimerRequest (void)
/****************************************************************************/
{
byte id;

for (id = 0; id < _N_GTIMERS; id++)
  {
  if (!Timer [id].req)
   {
    Timer [id].req = TRUE;
    break;
   }
  }

return (id);
}

/****************************************************************************/
byte gtimerReserve (byte id)
/****************************************************************************/
{
 if (id >= _N_GTIMERS)
  return (_N_GTIMERS);

 if (Timer [id].req)
  return (_N_GTIMERS);

 Timer [id].req = TRUE;

 return (id);
}

/****************************************************************************/
byte gtimerRelease (byte id)
/****************************************************************************/
{
if (id >= _N_GTIMERS)
  return (_N_GTIMERS);

init_Timer (id);

return (id);
}


/****************************************************************************/
void gtimerInitAndStart (byte id, dword count, boolean bAuto)
/****************************************************************************/
{
 if(Timer[id].req==FALSE)
  return;
 if(count<2)
  count = 2;
 Timer[id].count   = count;               // set count value
 Timer[id].count0  = count;               // set reload value
 Timer[id].bAuto = bAuto;                 // auto restarts at timeout
 Timer[id].timeout = FALSE;               // clears timeout condition
 Timer[id].running = TRUE;                // start timer
}

/****************************************************************************/
void gtimerRestart (byte id)
/****************************************************************************/
{
 gtimerInitAndStart(id, Timer[id].count0, Timer[id].bAuto);
}


/****************************************************************************/
void gtimerFreeze (byte id)
/****************************************************************************/
{
 Timer[id].running = FALSE;                   // stop running
}


/****************************************************************************/
void gtimerResume (byte id)
/****************************************************************************/
{
 if(Timer[id].req!=FALSE ||  (Timer[id].bAuto==FALSE && Timer[id].timeout))
  Timer[id].running = TRUE;     // start timer with last count
}

/****************************************************************************/
void gtimerFastForward (byte id)
/****************************************************************************/
{
 if(Timer[id].req!=FALSE ||  Timer[id].running!=FALSE)
  Timer[id].count=1;
}

/****************************************************************************/
boolean gtimerRunning (byte id)
/****************************************************************************/
{
 return (Timer [id].running);
}

/****************************************************************************/
boolean gtimerTO (byte id)
/****************************************************************************/
{
 boolean bRet=Timer[id].timeout;
 if(Timer[id].bAuto)
  Timer[id].timeout=FALSE;
 return (bRet);
}

/****************************************************************************/
dword gtimerGetTimeToGo (byte id)
/****************************************************************************/
{
 return (Timer [id].count);
}

#ifdef GTIMER_IMPLEMENTS_CALLBACK
/****************************************************************************/
void gtimerSetCallback (byte id, gtimerCallbackPtr pCallback, dword inValue, dword* pOutValue)
/****************************************************************************/
{
 Timer [id].pCallback = pCallback;
 Timer [id].inValue = inValue;
 Timer [id].pOutValue = pOutValue;
}

/****************************************************************************/
void gtimerSetCallbackInput (byte id, dword inValue )
/****************************************************************************/
{
 Timer [id].inValue = inValue;
}

/****************************************************************************/
void gtimerClearCallback (byte id)
/****************************************************************************/
{
 Timer [id].pCallback = NULL;
 Timer [id].inValue = 0;
 Timer [id].pOutValue = NULL;
}
#endif

/****************************************************************************/
/*                                                                          */
/*                  NOT EXPORTED FUNCTIONS                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
static void init_Timer (byte id)
/****************************************************************************/
{
 TimerType* pTimer=Timer+id;

 pTimer->req = FALSE;
 pTimer->count = 0;
 pTimer->count0 = 0;
 pTimer->running = FALSE;
 pTimer->bAuto = FALSE;
 pTimer->running = FALSE;
#ifdef IMPLEMENTS_TIMER_CALL_BACK
 gtimerClearCallback(id);
#endif

}


/****************************************************************************/
void gtimerOnTick (void)
/****************************************************************************/
{
 byte id;
 TimerType* pTimer=Timer;

 bTimerInterruptFired=FALSE;

 for (id = 0; id < _N_GTIMERS; id++, pTimer++)
  {
   if (pTimer->running && pTimer->count > 0)
    {
     pTimer->count--;
     if(pTimer->count == 0)
      {
        pTimer->timeout = TRUE;
        if(pTimer->bAuto)
         pTimer->count = pTimer->count0;
        else
         pTimer->running=FALSE;
    #ifdef GTIMER_IMPLEMENTS_CALLBACK
        if (pTimer->pCallback!=NULL)
         {
          dword outValue=pTimer->pCallback(id, pTimer->inValue);
          if(pTimer->pOutValue!=NULL)
          *pTimer->pOutValue = outValue;
         }
    #endif
      }
    }
  }
}


