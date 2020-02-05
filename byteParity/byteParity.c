#include "byteParity.h"

char __nibbleParity_LUT__[16];

/*
  0000 0
  0001 1
  0010 1
  0011 0
  0100 1
  0101 0
  0110 0
  0111 1
  1000 1
  1001 0
  1010 0
  1011 1
  1100 0
  1101 1
  1110 1
  1111 0 => 0x6996
*/

void byteOddParityInit()
{
 unsigned short generator = 0x6996;
 int i;
 for (i = 0; i < 16; i++)
  {
   if ((generator >> i) & 1)
    {
     __nibbleParity_LUT__[i] = 1;
    }
  }
}

int calcByteOddParity(unsigned char byte)
{
 return __nibbleParity_LUT__[byte & 15] ^ __nibbleParity_LUT__[byte >> 4];
}
