#include <stdio.h>

#include "shortIirLpF.h"

double tauArray[] = { 0,
                      0.632120559,
                      0.864664717,
                      0.950212932,
                      0.981684361,
                      0.993262053,
                      0.997521248,
                      0.999088118,
                      0.999664537,
                      0.99987659,
                      0.9999546 };

#define MAX_STEPS 10000

void main()
{
 short initial = 32767;
 short final = -32768;
 int divider;
 unsigned char tau;

 int n;
 tShortIIRLowPass f;
 short result;
 double converge;
 const int nbOfTauTested = sizeof(tauArray) / sizeof(*tauArray) - 1;

 printf("tau\t");
 for (n = 1; n <= nbOfTauTested; n++)
  {
   printf("  %- 5i%s", n, n < nbOfTauTested ? "\t" : "\n");
  }
 printf("%%  \t");
 for (n = 1; n <= nbOfTauTested; n++)
  {
   printf("%7.3f%s", tauArray[n] * 100.0, n < nbOfTauTested ? "\t" : "\n\n");
  }

 printf("from %hi to %hi\n", initial, final);
 printf("div\\tau\t");
 for (n = 1; n <= nbOfTauTested; n++)
  {
   printf("% 4i\t", n);
  }
 printf("converge\n");

 for (divider = 1; divider <= 16; divider++)
  {
   shortIIRLowPassInit(divider, initial, &f);
   tau = 1;
   printf("%- 4i\t", divider);
   for (n = 1; n <= MAX_STEPS; n++)
    {
     shortIIRLowPassInput(final, &f);
     result = shortIIRLowPassGet(&f);
     converge = (double)(result - final) / (double)final;
     if (converge < 0.0)
      {
       converge = -converge;
      }
     converge = 1.0 - converge;
     if (tau <= nbOfTauTested && converge >= tauArray[tau])
      {
       printf("% 4i\t", n);
       tau++;
      }
     if (result == final)
      {
       for (; tau <= nbOfTauTested; tau++)
        {
         printf("% 4i\t", n);
        }
       printf("% 4i\n", n);
       break;
      }
    }
   if (n > MAX_STEPS)
    {
     printf("error! no convergence after %i steps", MAX_STEPS);
    }
  }
 result = -256;
}
