#include <stdio.h>

#include "byteParity.h"

int slowCalculateOddity(unsigned char byte)
{
 char count, i;
 for (i = count = 0; i < 8; i++)
  {
   if (byte & (1 << i))
    {
     count++;
    }
  }
 return count % 2;
}

void main()
{
 short byte;
 unsigned char expected;
 byteOddParityInit();
 printf("LUT: odd parity for nibble at\n");
 for (byte = 0; byte < sizeof(__nibbleParity_LUT__); byte++)
  {
   printf(" 0x%1x: %i\n", byte, __nibbleParity_LUT__[byte]);
  }
 printf("** tests **\n");
 for (byte = 0; byte < 256; byte++)
  {
   int quick = calcByteOddParity(byte);
   int slow = slowCalculateOddity(byte);
   if (quick != slow)
    {
     printf("failure for byte 0x%02hx: oddity should be %i and calcOddParity provided %i\n",
            byte, slow, quick);
     return;
    }
   if (CALC_BYTE_ODD_PARITY(byte) != slow)
    {
     printf("failure for byte 0x%02hx: oddity should be %i and CALC_BYTE_ODD_PARITY provided %i\n",
            byte, slow, CALC_BYTE_ODD_PARITY(byte));
     return;
    }
   expected = byte & 127;
   if (byteWithParity(byte, 0) != expected)
    {
     printf("failure for byte 0x%02hx: byteWithParity(byte, 0) expected to return 0x%02hx\n",
            byte, (short)expected);
     return;
    }
   expected |= slowCalculateOddity(expected) ? 0 : 0x80;
   if (byteWithParity(byte, 1) != expected)
    {
     printf("failure for byte 0x%02hx: byteWithParity(byte, 1) expected to return 0x%02hx\n",
            byte, (short)expected);
     return;
    }
   expected ^= 0x80;
   if (byteWithParity(byte, 2) != expected)
    {
     printf("failure for byte 0x%02hx: byteWithParity(byte, 2) expected to return 0x%02hx\n",
            byte, (short)expected);
     return;
    }
  }
 printf("** done **\n");
}
