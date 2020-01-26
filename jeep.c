#include "jeep.h"

#include <string.h>

dword getBitsInDword(dword dw, byte width, byte bitOffset)
{
 dword mask;
 if (bitOffset > 31) // shifts on dwords of more than 31 are undefined in C
  {
   return 0;
  }
 if (width > 31)
  {
   mask = 0xffffffff;
  }
 else
  {
   mask = (1ul << width) - 1ul;
  }
 dw >>= bitOffset;
 dw &= mask;
 return dw;
}

dword setBitsInDword(dword dw, dword value, byte width, byte bitOffset)
{
 dword mask;
 if (bitOffset < 32)
  {
   if (width > 31)
    {
     mask = 0xffffffff; // we're careful: (1ul<<32) is undefined in C
    }
   else
    {
     mask = ((1ul << width) - 1ul) << bitOffset;
    }
   dw &= ~mask;
   dw |= (value << bitOffset) & mask;
  }
 return dw;
}

/*
 * converts ASCII char input as an hexadecimal digit [0-9A-Fa-f]
 * to a 0 to 15 returned value
 * or 255 if input char is not is the requested range
 */
byte ascii1byte(byte Ascii)
{
 Ascii -= '0';
 if (Ascii > 9)
  {
   Ascii -= 'A' - '0' - 10;
   if (Ascii < 10 || Ascii > 15)
    {
     Ascii -= 'a' - 'A';
     if (Ascii < 10 || Ascii > 15)
      {
       Ascii = 0xff;
      }
    }
  }
 return Ascii;
}

/*
 * replaces 'oldS' string to 'newS' string in str maxOcc times
 * if maxOcc is less than 1, replaces as many times as possible
 * return: number of occurrences found
 * caveat: make sure str has enough room (no control is made on this)
 */
int multipleFindAndReplace(char* str, const char* oldS, const char* newS, int maxOcc)
{
 char* pf; // pointer to oldS found in str
 size_t l0 = strlen(oldS);
 size_t l1 = strlen(newS);
 int nbFound = 0;
 for (pf = str; (pf = strstr(pf, oldS)) != NULL; pf += l1)
  {
   nbFound++;
   memmove(pf + l1, pf + l0, strlen(str) - l0 - (pf - str) + 1);
   memcpy(pf, newS, l1);
   if (maxOcc > 0 && nbFound >= maxOcc)
    {
     break;
    }
  }
 return nbFound;
}

/* ---------------------------------
 * Test Section
 * --------------------------------- */
#ifdef JEEP_TESTS

int test_getSetBitsInDword_step(dword v, byte w, byte b, dword expected)
{
 dword cv = getBitsInDword(v, w, b);
 dword testV = 0x55aa55aa;
 byte w2 = w;
 if (b + w > 32)
  {
   w2 -= b + w - 32;
  }
 if (cv != expected)
  {
   return 1;
  }
 cv = setBitsInDword(v, testV, w, b);
 if (getBitsInDword(cv, w, b) != (testV & ((1 << w2) - 1)))
  {
   return 2;
  }
 cv = setBitsInDword(v, expected, w, b);
 if (cv != v)
  {
   return 3;
  }
 if (v != getBitsInDword(v, 32, 0))
  {
   return 4;
  }
 if (setBitsInDword(v, 0, 32, 0) != 0)
  {
   return 5;
  }
 return 0;
}

struct test_getSetBitsInDword_vector {
 dword v; byte w; byte bOffset; dword expected;
};

/*
 * tests both getBitsInDword and setBitsInDword
 * returns 0 if ok or a value which can be broken down as 100*s+r, with
 * s between the failure step and r the return code of test_getSetBitsInDword_step
 */
int test_getSetBitsInDword()
{
 static struct test_getSetBitsInDword_vector testArray[] = {
  { 0x1f04, 8, 4, 0xf0 },
  { 0x12345678, 7, 11, 0xa },
  { 0x87654321, 11, 11, 0x4a8 },
  { 0x12345678, 4, 0, 0x8 },
  { 0x12345678, 8, 24, 0x12 },
  { 0x12345678, 12, 25, 0x9 },
  { 0x12345678, 16, 15, 0x2468 }
 };

 int r = 0, i;
 for (i = 0; i < sizeof(testArray) / sizeof(*testArray); i++)
  {
   r = test_getSetBitsInDword_step(testArray[i].v, testArray[i].w, testArray[i].bOffset, testArray[i].expected);
   if (r)
    {
     return 100 * (i + 1) + r;
    }
  }
 return r;
}

struct test_multipleFindAndReplace_vector {
 char str[64]; const char* oldS; const char* newS; int maxOcc;
 const char* expectedStr; int expectedNb;
};

/* will return 0 if test ok, or step where test failed */
int test_multipleFindAndReplace()
{
 static struct test_multipleFindAndReplace_vector tArr[] =
 {
  { "foo is foo and not foo", "foo", "bar", 0, "bar is bar and not bar", 3 },
  { "foo is foo and not foo", "foo", "quux", 1, "quux is foo and not foo", 1 },
  { "foo is foo and not foo", "o", "0", -1, "f00 is f00 and n0t f00", 7 },
  { "foo is foo and not foo", "o", "!!", -2, "f!!!! is f!!!! and n!!t f!!!!", 7 },
  { "foo is foo and not foo", "o", "", -3, "f is f and nt f", 7 },
  { "foo is foo and not foo", "FOO", "BAR", -1, "foo is foo and not foo", 0 }
 }, * pTest;
 int i, r;
 for (pTest = tArr, i = 0; i < sizeof(tArr) / sizeof(*tArr); i++, pTest++)
  {
   r = multipleFindAndReplace(pTest->str, pTest->oldS, pTest->newS, pTest->maxOcc);
   if (r != pTest->expectedNb || strcmp(pTest->str, pTest->expectedStr))
    {
     return i + 1;
    }
  }
 return 0;
}

/*
 * will return 0 if okay, or 256*chc+stp, chc being the faulty ascii code [0-255]
 * and stp the step which failed in the test procedure
 */
word test_ascii1byte(void)
{
 int i;
 word r;
 byte b;
 for (i = r = 0; r == 0 && i < 255; i++)
  {
   b = ascii1byte((char)i);
   if (i >= '0' && i <= '9')
    {
     if (b != i - '0')
      {
       r = 1;
      }
    }
   else if (i >= 'A' && i <= 'F')
    {
     if (b != i - 'A' + 10)
      {
       r = 2;
      }
    }
   else if (i >= 'a' && i <= 'f')
    {
     if (b != i - 'a' + 10)
      {
       r = 3;
      }
    }
   else if (b != 0xff)
    {
     r = 4;
    }
  }
 if (r)
  {
   r += (word)i << 8;
  }
 return r;
}

#endif
