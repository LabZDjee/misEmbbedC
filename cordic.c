/******************************************************************************
  FILENAME: Cordic.c

 *****************************************************************************
  Public domain module

  Adaption:       Gerard Gauthier
  Author:         J. Pitts Jarvis III (+), 3Com Corporation, October 1990
  Target Device:  Portable

  (+) very sadly, J. Pitts Jarvis III born in Fayetteville, Arkansas on November 3, 1946
      died suddenly of a heart attack on October 10, 2003, in Palo Alto, California.

 * cordic.c computes CORDIC (COordinate, Rotation DIgital Computer)
 * constants and exercises the basic algorithms
 * Represents all numbers in fixed point notation, as a 32-bit long integer:
 *  1 bit sign, (31-n) bits for integral part, and n bits for fractional part
 *  for example, n=29 lets us represent numbers in the interval [-4, 4[ in 32 bit
 * Two's complement arithmetic is operative here

  Usage: fast sine, cosine, arctg, sinh, cosh, ln, sqrt, etc

 * *** cordic algorithm identities ***
 *
 * for circular functions, starting with [x, y, z] and then
 *  driving z to 0 gives: [P*(x*cos(z)-y*sin(z)), P*(y*cos(z)+x*sin(z)), 0]: Circular (C)
 *  driving y to 0 gives: [P*sqrt(x^2+y^2), 0, z+atan(y/x)]: Reciprocal Circular (rC)
 *  where K = 1/P = sqrt(1+1)* . . . *sqrt(1+(2^(-2*i)))
 *  special cases which compute interesting functions
 *   sin, cos       C[K*r, 0, a] -> [r*cos(a), r*sin(a), 0]
 *   atan          rC[1, a, 0]   -> [sqrt(1+a^2)/K, 0, atan(a)]
 *                 rC[x, y, 0]   -> [sqrt(x^2+y^2)/K, 0, atan(y/x)]
 *
 * for hyperbolic functions, starting with [x, y, z] and then
 *  driving z to 0 gives: [P*(x*cosh(z)+y*sinh(z)), P*(y*cosh(z)+x*sinh(z)), 0]: Hyperbolic (H)
 *  driving y to 0 gives: [P*sqrt(x^2-y^2), 0, z+atanh(y/x)]: Reciprocal Hyperbolic (rH)
 *  where K = 1/P = sqrt(1-(1/2)^2)* . . . *sqrt(1-(2^(-2*i)))
 *  special cases which compute interesting functions
 *   sinh, cosh     H[K, 0, a] -> [cosh(a), sinh(a), 0]
 *   exponential    H[K, K, a] -> [e^a, e^a, 0]
 *   atanh         rH[1, a, 0] -> [sqrt(1-a^2)/K, 0, atanh(a)]
 *                 rH[x, y, 0] -> [sqrt(x^2-y^2)/K, 0, atanh(y/x)]
 *   ln            rH[a+1, a-1, 0] -> [2*sqrt(a)/K, 0, ln(a)/2]
 *   sqrt          rH[a+(K/2)^2, a-(K/2)^2, 0] -> [sqrt(a), 0, ln(a*(2/K)^2)/2]
 *   sqrt, ln      rH[a+(K/2)^2, a-(K/2)^2, -ln(K/2)] -> [sqrt(a), 0, ln(a)/2]
 *
 * for linear functions, starting with [x, y, z] and then
 *  driving z to 0 gives: [x, y+x*z, 0]: Linear
 *  driving y to 0 gives: [x, 0, z+y/x]: Reciprocal Linear

 Example: compute 2-D coordinates from rectangular (x, y) to polar (r, th)
  long x, y, r, th;
  x = 110; // init x, unscaled integer value (not yet a cordic value)
  y = -96; // init y, unscaled integer value (not yet a cordic value)
  // here multiplies by cordic_X0C will both counterbalance 1/K and scale x and y
  cordic_InvertCircular(x*cordic_X0C, y*cordic_X0C, 0); // sqrt(x*2+y^2)/K, 0, atan(y/x)
  r = cordic_X; // module, scaled cordic 32-bit decimal value
  th = cordic_Z; // angle, scaled cordic 32-bit decimal value
 And from this, calculate back from polar to rectangular
  cordic_Circular(cordic_X0C, 0, th);  // cos(th), sin(th), 0
  // now, calculate x and y as scaled cordic 32-bit decimal values
  // right shifts to ensure it's a 16x16 multiply
  //  shifts biased to save sine/cosine significant fractional digits
  x = (r>>(cordic_fractionBits-3)) * (cordic_X>>3);
  y = (r>>(cordic_fractionBits-3)) * (cordic_Y>>3);
  // results of this round-trip (for cordic_fractionBits=16):
  //  x = 109.99902, y = -96.00854

  Change Log is at end of file

*******************************************************************************/

//****************************************
//  include files
//****************************************

#include "Cordic.h"

/*******************************************************************************************
 * local defines that remove the "cordic_" prefix to alleviate implementation in this module
 *******************************************************************************************/

#define X cordic_X
#define Y cordic_Y
#define Z cordic_Z
#define X0C cordic_X0C
#define X0H cordic_X0H
#define X0R cordic_X0R
#define OneOverE cordic_OneOverE
#define E cordic_E
#define HalfLnX0R cordic_HalfLnX0R
#define fractionBits cordic_fractionBits
#define longBits cordic_longBits
#define One cordic_One
#define HalfPi cordic_HalfPi

/*******************************************************************************************/
/* misc local defines */

/* Delta is inefficient but pedagogical */
#define Delta(n, Z) (Z >= 0) ? (n) : -(n)
#define abs(n) (n >= 0) ? (n) : -(n)

/*******************************************************************************************
 * exported variables - BEWARE: their external names are prefixed with cordic_
 *******************************************************************************************/

/* X, Y, Z result registers of cordic functions */
long X, Y, Z;

/***************************************
 * seeds for circular and hyperbolic
 ***************************************/
/* 'K' prescaling constant for circular functions */
/*   cordic_Circular(X0C, 0, a) -> cos(a), sin(a), 0 */
long X0C;
/* 'K' prescaling constant for hyperbolic functions */
/*  cordic_Hyperbolic(X0H, 0, a) -> cosh(a), sinh(a), 0 */
long X0H;
/* constant useful for reciprocal hyperbolic function: (X0H/2)^2 */
/*   cordic_InvertHyperbolic(a+1, a-1, 0) -> 2*sqrt(a)/X0R, 0, ln(a)/2 */
long X0R; /* 'K' prescaling constant for reciprocal hyperbolic functions */

/* e: base of natural logarithms */
long E;
/* 1/e */
long OneOverE;

/* constant used in simultaneous sqrt and ln computations: -ln(X0H/2) */
/*  cordic_InvertHyperbolic(x+X0R, x-X0R, cordic_HalfLnX0R) -> sqrt(a), 0, ln(a)/2 */
long HalfLnX0R;     /* constant used in simultaneous sqrt, ln computation */

/*****************************************
 * Local variables
 *****************************************/

/* compute atan(x) and atanh(x) using infinite series
 *    atan(x) =  x - x^3/3 + x^5/5 - x^7/7 + . . . for x^2 < 1
 *    atanh(x) = x + x^3/3 + x^5/5 + x^7/7 + . . . for x^2 < 1
 * To calculate these functions to 32 bits of precision, pick
 * terms[i] s.t. ((2^-i)^(terms[i]))/(terms[i]) < 2^-32
 * For x <= 2^(-11), atan(x) = atanh(x) = x with 32 bits of accuracy  */
static unsigned terms[11] = { 0, 27, 14, 9, 7, 5, 4, 4, 3, 3, 3 };
static long a[28], atan[fractionBits + 1], atanh[fractionBits + 1];

/*****************************************************************************
    FUNCTION  cordic_Reciprocal

    Description:
      calculate reciprocal of n to k bits of precision
      a and r form integer and fractional parts of the dividend respectively

    Entrance Conditions:

    Exit Conditions:

    args:
      n: computed number
      k: number of bits

    returns:
      reciprocal
 *****************************************************************************/
long cordic_Reciprocal(unsigned n, unsigned k)
{
 unsigned i, a = 1;
 long r = 0;
 for (i = 0; i <= k; ++i)
  {
   r += r;
   if (a >= n)
    {
     r += 1;
     a -= n;
    }
   a += a;
  }
 return a >= n ? r + 1 : r; /* round result */
}

/*****************************************************************************
    FUNCTION  cordic_ScaledReciprocal

    Description:
     calculate scaled reciprocal (1/n) of n to k bits of precision
     beware: internal routine, does not deal correctly with n<=0

    Entrance Conditions:

    Exit Conditions:

    args:
     n: computed number
     k: number of bits

    returns:
      scaled reciprocal
 *****************************************************************************/
long cordic_ScaledReciprocal(long n, unsigned k)
{
 long a, r = 0;
 unsigned i;
 a = 1L << k;
 for (i = 0; i <= k; ++i)
  {
   r += r;
   if (a >= n)
    {
     r += 1;
     a -= n;
    }
   a += a;
  }
 return a >= n ? r + 1 : r; /* round result */
}

/*****************************************************************************
    FUNCTION  cordic_Poly2

    Description:
      calculates polynomial where the variable is an integral power of 2
      coefficients are in the array a[]

    Entrance Conditions:

    Exit Conditions:

    args:
     log is the power of 2 of the variable
     n is the order of the polynomial

    returns:
      polynomial
 *****************************************************************************/
long cordic_Poly2(int log, unsigned n)
{
 long r = 0; int i;
 for (i = n; i >= 0; --i)
  {
   r = (log<0 ? r> > -log : r << log) + a[i];
  }
 return r;
}

/*****************************************************************************
    FUNCTION  cordic_Circular

    Description:
     First Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Z -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_Circular(long x, long y, long z)
{
 int i;
 X = x; Y = y; Z = z;
 for (i = 0; i <= fractionBits; ++i)
  {
   x = X >> i;
   y = Y >> i;
   z = atan[i];
   X -= Delta(y, Z);
   Y += Delta(x, Z);
   Z -= Delta(z, Z);
  }
}

/*****************************************************************************
    FUNCTION  cordic_InvertCircular

    Description:
     Reciprocal of first Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Y -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_InvertCircular(long x, long y, long z)
{
 int i;
 X = x; Y = y; Z = z;
 for (i = 0; i <= fractionBits; ++i)
  {
   x = X >> i;
   y = Y >> i;
   z = atan[i];
   X -= Delta(y, -Y);
   Z -= Delta(z, -Y);
   Y += Delta(x, -Y);
  }
}

/*****************************************************************************
    FUNCTION  cordic_Hyperbolic

    Description:
     Second Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Z -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_Hyperbolic(long x, long y, long z)
{
 int i;
 X = x; Y = y; Z = z;
 for (i = 1; i <= fractionBits; ++i)
  {
   x = X >> i;
   y = Y >> i;
   z = atanh[i];
   X += Delta(y, Z);
   Y += Delta(x, Z);
   Z -= Delta(z, Z);
   if ((i == 4) || (i == 13))
    {
     x = X >> i;
     y = Y >> i;
     z = atanh[i];
     X += Delta(y, Z);
     Y += Delta(x, Z);
     Z -= Delta(z, Z);
    }
  }
}

/*****************************************************************************
    FUNCTION  cordic_InvertHyperbolic

    Description:
     Reciprocal of second Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Y -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_InvertHyperbolic(long x, long y, long z)
{
 int i;
 X = x; Y = y; Z = z;
 for (i = 1; i <= fractionBits; ++i)
  {
   x = X >> i;
   y = Y >> i;
   z = atanh[i];
   X += Delta(y, -Y);
   Z -= Delta(z, -Y);
   Y += Delta(x, -Y);
   if ((i == 4) || (i == 13))
    {
     x = X >> i; y = Y >> i; z = atanh[i];
     X += Delta(y, -Y);
     Z -= Delta(z, -Y);
     Y += Delta(x, -Y);
    }
  }
}

/*****************************************************************************
    FUNCTION  cordic_Linear

    Description:
     Third Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Z -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_Linear(long x, long y, long z)
{
 int i;
 X = x;
 Y = y;
 Z = z;
 z = One;
 for (i = 1; i <= fractionBits; ++i)
  {
   x >>= 1;
   z >>= 1;
   Y += Delta(x, Z);
   Z -= Delta(z, Z);
  }
}

/*****************************************************************************
    FUNCTION  cordic_InvertLinear

    Description:
     Reciprocal of third Cordic generic function
     For more see Cordic identities at the beginning of this file

    Entrance Conditions:
     Cordic Module Initialized

    Exit Conditions:
     X, Y, Z updated as result of computation (Y -> 0)

    args:
     x, y, z: entry variables

    returns:
      nothing
 *****************************************************************************/
void cordic_InvertLinear(long x, long y, long z)
{
 int i;
 X = x;
 Y = y;
 Z = z;
 z = One;
 for (i = 1; i <= fractionBits; ++i)
  {
   Z -= Delta(z >>= 1, -Y);
   Y += Delta(x >>= 1, -Y);
  }
}

/*****************************************************************************
    FUNCTION  cordic_Init

    Description:
     Cordic Initialization. To be called once prior to Coridc function (Circular, Hyperbolic...)

    Entrance Conditions:

    Exit Conditions:
     Circular, Hyperbolic, and Linear Cordic function can be safely called

    args:

    returns:
      nothing
 *****************************************************************************/
void cordic_Init(void)
{
 int i;
 for (i = 0; i <= 13; ++i)
  {
   a[2 * i] = 0;
   a[2 * i + 1] = cordic_Reciprocal(2 * i + 1, fractionBits);
  }
 for (i = 0; i <= 10; ++i)
  {
   atanh[i] = cordic_Poly2(-i, terms[i]);
  }
 atan[0] = HalfPi / 2; /* atan(2^0)= pi/4 */
 for (i = 1; i <= 7; ++i)
  {
   a[4 * i - 1] = -a[4 * i - 1];
  }
 for (i = 1; i <= 10; ++i)
  {
   atan[i] = cordic_Poly2(-i, terms[i]);
  }
 for (i = 11; i <= fractionBits; ++i)
  {
   atan[i] = atanh[i] = 1L << (fractionBits - i);
  }
 cordic_Circular(One, 0L, 0L);
 X0C = cordic_ScaledReciprocal(X, fractionBits);
 cordic_Hyperbolic(One, 0L, 0L);
 X0H = cordic_ScaledReciprocal(X, fractionBits);
 X0R = X0H >> 1;
 cordic_Linear(X0R, 0L, X0R);
 X0R = Y;
 cordic_Hyperbolic(X0H, X0H, -One);
 OneOverE = X;
 cordic_Hyperbolic(X0H, X0H, One);
 E = X;
 cordic_InvertHyperbolic(One + X0R, One - X0R, 0L);
 HalfLnX0R = Z;
}

#ifdef CORDIC_TEST_FCT

/*****************************************************************************
    FUNCTION  cordic_WriteFraction

    Description:
      test function: print n as NET.FRACT to fp
      integer and fractional parts separated by a dot
    Entrance Conditions:

    Exit Conditions:

    args:
     n value
     fp file where to print n to

    returns:
      nothing
 *****************************************************************************/
void cordic_fWriteFraction(long n, FILE* fp)
{
 unsigned short i, low, digit;
 unsigned long k;
 putchar(n < 0 ? '-' : ' ');
 n = abs(n);
 fprintf(fp, "%li", n >> fractionBits);
 fputc('.', fp);
 k = n << (longBits - fractionBits); /* align octal point at left */
 low = (unsigned short)k;
 k >>= 4;       /* shift to make room for a decimal digit */
 for (i = 1; i <= 8; ++i)
  {
   digit = (k *= 10L) >> (longBits - 4);
   low = (low & 0xf) * 10;
   k += ((unsigned long)(low >> 4)) - ((unsigned long)digit << (longBits - 4));
   fputc(digit + '0', fp);
  }
}

/*****************************************************************************
    FUNCTION  cordic_WriteRegisters

    Description:
      test function: prints main registers X, Y, Z of this Cordic implementation
      to fp

    Entrance Conditions:

    Exit Conditions:

    args:
     fp file where to print n to

    returns:
      nothing
 *****************************************************************************/
void cordic_fWriteRegisters(FILE* fp)
{
 fprintf(fp, "  X: "); cordic_WriteVar(X);
 fprintf(fp, "  Y: "); cordic_WriteVar(Y);
 fprintf(fp, "  Z: "); cordic_WriteVar(Z);
}

/*****************************************************************************
    FUNCTION  cordic_WriteVar

    Description:
      test function: sends data about a variable to stdout
      (decimal value, hexa raw value, etc)

    Entrance Conditions:

    Exit Conditions:

    args:
     n value to print

    returns:
      nothing
 *****************************************************************************/
void cordic_fWriteVar(long n, FILE* fp)
{
 cordic_fWriteFraction(n, fp);
 fprintf(fp, "  %li 0x%08lx\n", n, n);
}

/*****************************************************************************
    FUNCTION  cordic_Test

    Description:
     Generic test functions together with their expected results in some cases

    Entrance Conditions:

    Exit Conditions:

    args:

    returns:
      nothing
 *****************************************************************************/
void cordic_Test(void)
{
 int i;
 long r;

 cordic_Init();
 printf("\natanh(2^-n)\n");
 for (i = 1; i <= 10; ++i)
  {
   printf("%2d ", i);
   cordic_WriteVar(atanh[i]);
  }
 r = 0;
 for (i = 1; i <= fractionBits; ++i)
  {
   r += atanh[i];
  }
 r += atanh[4] + atanh[13];
 printf("radius of convergence"); cordic_WriteFraction(r); printf("\n\natan(2^-n)\n");
 for (i = 0; i <= 10; ++i)
  {
   printf("%2d ", i);
   cordic_WriteVar(atan[i]);
  }
 r = 0;
 for (i = 0; i <= fractionBits; ++i)
  {
   r += atan[i];
  }
 printf("radius of convergence"); cordic_WriteFraction(r);

 /* all the results reported in the printfs are calculated with an HP-41C */
 printf("\n\n--------------------circular functions--------------------\n");
 printf("Grinding on [1, 0, 0]\n");
 cordic_Circular(One, 0L, 0L); cordic_WriteRegisters;
 printf("\n  K: "); cordic_WriteVar(cordic_ScaledReciprocal(X, fractionBits));
 printf("\nGrinding on [K, 0, 0]\n");
 cordic_Circular(X0C, 0L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [K, 0, pi/6] -> [0.86602540, 0.50000000, 0]\n");
 cordic_Circular(X0C, 0L, HalfPi / 3L); cordic_WriteRegisters;
 printf("\nGrinding on [K, 0, pi/4] -> [0.70710678, 0.70710678, 0]\n");
 cordic_Circular(X0C, 0L, HalfPi / 2L); cordic_WriteRegisters;
 printf("\nGrinding on [K, 0, pi/3] -> [0.50000000, 0.86602540, 0]\n");
 cordic_Circular(X0C, 0L, 2L * (HalfPi / 3L)); cordic_WriteRegisters;
 printf("\n------Inverse functions------\n");
 printf("Grinding on [1, 0, 0]\n");
 cordic_InvertCircular(One, 0L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [1, 1/2, 0] -> [1.84113394, 0, 0.46364761]\n");
 cordic_InvertCircular(One, One / 2L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [2, 1, 0] -> [3.68226788, 0, 0.46364761]\n");
 cordic_InvertCircular(One * 2L, One, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [1, 5/8, 0] -> [1.94193815, 0, 0.55859932]\n");
 cordic_InvertCircular(One, 5L * (One / 8L), 0L); cordic_WriteRegisters;
 printf("\nGrinding on [1, 1, 0] -> [2.32887069, 0, 0.78539816]\n");
 cordic_InvertCircular(One, One, 0L); cordic_WriteRegisters;
 printf("\n--------------------hyperbolic functions--------------------\n");
 printf("Grinding on [1, 0, 0]\n");
 cordic_Hyperbolic(One, 0L, 0L); cordic_WriteRegisters;
 printf("\n  K: "); cordic_WriteVar(cordic_ScaledReciprocal(X, fractionBits));
 printf("  R: "); cordic_Linear(X0R, 0L, X0R); cordic_WriteVar(Y);
 printf("\nGrinding on [K, 0, 0]\n");
 cordic_Hyperbolic(X0H, 0L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [K, 0, 1] -> [1.54308064, 1.17520119, 0]\n");
 cordic_Hyperbolic(X0H, 0L, One); cordic_WriteRegisters;
 printf("\nGrinding on [K, K, -1] -> [0.36787944, 0.36787944, 0]\n");
 cordic_Hyperbolic(X0H, X0H, -One); cordic_WriteRegisters;
 printf("\nGrinding on [K, K, 1] -> [2.71828183, 2.71828183, 0]\n");
 cordic_Hyperbolic(X0H, X0H, One); cordic_WriteRegisters;
 printf("\n------Inverse functions------\n");
 printf("Grinding on [1, 0, 0]\n");
 cordic_InvertHyperbolic(One, 0L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [1/e + 1, 1/e - 1, 0] -> [1.00460806, 0, -0.50000000]\n");
 cordic_InvertHyperbolic(OneOverE + One, OneOverE - One, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [e + 1, e - 1, 0] -> [2.73080784, 0, 0.50000000]\n");
 cordic_InvertHyperbolic(E + One, E - One, 0L); cordic_WriteRegisters;
 printf("\nGrinding on (1/2)*ln(3) -> [0.71720703, 0, 0.54930614]\n");
 cordic_InvertHyperbolic(One, One / 2L, 0L); cordic_WriteRegisters;
 printf("\nGrinding on [3/2, -1/2, 0] -> [1.17119417, 0, -0.34657359]\n");
 cordic_InvertHyperbolic(One + (One / 2L), -(One / 2L), 0L); cordic_WriteRegisters;
 printf("\nGrinding on sqrt(1/2) -> [0.70710678, 0, 0.15802389]\n");
 cordic_InvertHyperbolic(One / 2L + X0R, One / 2L - X0R, 0L); cordic_WriteRegisters;
 printf("\nGrinding on sqrt(1) -> [1.00000000, 0, 0.50449748]\n");
 cordic_InvertHyperbolic(One + X0R, One - X0R, 0L); cordic_WriteRegisters;
 printf("\nGrinding on sqrt(2) -> [1.41421356, 0, 0.85117107]\n");
 cordic_InvertHyperbolic(One * 2L + X0R, One * 2L - X0R, 0L); cordic_WriteRegisters;
 printf("\nGrinding on sqrt(1/2), ln(1/2)/2 -> [0.70710678, 0, -0.34657359]\n");
 cordic_InvertHyperbolic(One / 2L + X0R, One / 2L - X0R, -HalfLnX0R); cordic_WriteRegisters;
 printf("\nGrinding on sqrt(3)/2, ln(3/4)/2 -> [0.86602540, 0, -0.14384104]\n");
 cordic_InvertHyperbolic((3L * One / 4L) + X0R, (3L * One / 4L) - X0R, -HalfLnX0R);
 cordic_WriteRegisters;
 printf("\nGrinding on sqrt(2), ln(2)/2 -> [1.41421356, 0, 0.34657359]\n");
 cordic_InvertHyperbolic(One * 2L + X0R, One * 2L - X0R, -HalfLnX0R);
 cordic_WriteRegisters;
}

#endif /* CORDIC_TEST_FCT */

/******************************************************************************
  End of file
*******************************************************************************/
