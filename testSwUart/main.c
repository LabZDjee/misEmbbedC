#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************

file: main.c
purpose: simple demo that demonstrates the swUart module (software uart)

**************************************************/
#include "timer.h"
#include "gtimer.h"
#include "rs232.h"
#include "swUart.h"

void timer_tick(void);

int var = 0;

int portComIdx = 2; // COM{x} with {x} = portComIdx+1

void setTxOutput(swUartDataStateE s)
{
 if (s == swUSpace_Low)
  {
   RS232_enableRTS(portComIdx);
  }
 else
  {
   RS232_disableRTS(portComIdx);
  }
}

swUartDataStateE getRxInput(void)
{
 return RS232_IsCTSEnabled(portComIdx) ? swUSpace_Low : swUMark_High;
}

int main(void)
{
 const byte uartIdx = 0;
 swUartConfigurationT swUartCfg = { nbBits: 8, parity:swUEvenParity, stop:swU1Stop, bitWidth:4, bTripleScan:FALSE };
 if (start_timer(2, timer_tick))
  {
   printf("timer error\n");
   return 1;
  }
 if (RS232_OpenComport(portComIdx, 38400, "8N1") != 0)
  {
   printf("RS232_OpenComport (#%i) error\n", portComIdx);
   return 1;
  }
 gtimerInitModule();
 gtimerReserve(SWUART1_SEND_TIMER_ID);
 gtimerReserve(SWUART1_RECEIVE_TIMER_ID);
 if (!swUartSendInit(uartIdx, &swUartCfg, SWUART1_SEND_TIMER_ID, setTxOutput))
  {
   printf("Error init of Tx unit\n");
   return 1;
  }

 if (!swUartReceiveInit(uartIdx, &swUartCfg, SWUART1_RECEIVE_TIMER_ID, getRxInput))
  {
   printf("Error init of Rx unit\n");
   return 1;
  }
 printf("Press ctrl-c to quit\n");

 while (1)
  {
   static word index = 0;
   static char* pCh = "Hello world!\n";
   static boolean bUsePeek = FALSE;
   byte n;
   word ch;

   if (bTimerInterruptFired)    /* flag coming from timer interrupt */
    {
     gtimerOnTick();     /* manages gTimer */
    }
   /* send text gradually until returns TRUE, indicating all is done*/
   if (swUartSendData(uartIdx, pCh, strlen(pCh), &index))
    {
     index = 0;   /* sends over an over again */
    }
   swUartReceiveScanForStart(0);      /* scans start condition which will start Rx stat machine */
   if (swUartReceiveGetAndClearError(uartIdx, FALSE))     /* scan errors */
    {
     /* this one will read errors and clear them */
     printf("Errors!!! Value: %i\n", (int)swUartReceiveGetAndClearError(uartIdx, TRUE));
    }
   if (bUsePeek)     /* will read inbound characters through 'peek' mechanism */
    {
     /* will peek until we got a LF */
     n = swUartHowManyChars(uartIdx);
     if (n > 0)
      {
       if (swUartPeekChar(uartIdx) == '\n')
        {
         /* we got the LF: get string through peekNChar function and flush */
         for (; n > 0; n--)
          {
           ch = swUartPeekNChar(uartIdx, n - 1);
           printf("Peeked: '%c' - 0x%03x\n", ch, ch);
          }
         swUartFlushChars(uartIdx);
         bUsePeek = FALSE; /* switch to getChar method for next string to fetch */
        }
      }
    }
   else
    {
     ch = swUartGetChar(uartIdx);
     if (ch != 0xffff)  /* char is valid */
      {
       printf("Received: '%c' - 0x%03x\n", ch, ch);
       if (ch == '\n') /* got entire string: switch to peek method */
        {
         bUsePeek = TRUE;
        }
      }
    }
  }

 stop_timer();

 return 0;
}

void timer_tick(void)
{
 bTimerInterruptFired = TRUE;
}
