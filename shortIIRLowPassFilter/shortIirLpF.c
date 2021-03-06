#include "shortIirLpF.h"

void shortIIRLowPassInit(unsigned char divider, short initial, tShortIIRLowPass* pFilter)
{
 if (divider == 0)
  {
   divider = 1;
  }
 pFilter->divider = divider;
 pFilter->halfDivider = divider >> 1;
 pFilter->acc = (long)initial * (long)divider;
}

void shortIIRLowPassInput(short newValue, tShortIIRLowPass* pFilter)
{
 long takeOut = pFilter->acc;
 if (takeOut >= 0)
  {
   takeOut = (takeOut + (long)pFilter->halfDivider) / (long)pFilter->divider;
  }
 else
  {
   takeOut = -((pFilter->halfDivider - takeOut) / pFilter->divider);
  }
 pFilter->acc = (long)newValue + pFilter->acc - takeOut;
}

short shortIIRLowPassGet(tShortIIRLowPass* pFilter)
{
 long acc = pFilter->acc;
 if (acc >= 0)
  {
   return (short)((acc + (long)pFilter->halfDivider) / (long)pFilter->divider);
  }
 else
  {
   return (short)-((pFilter->halfDivider - acc) / pFilter->divider);
  }
}
