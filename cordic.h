/******************************************************************************
  FILENAME: Cordic.h

 *****************************************************************************
  Public domain module

  Adaption:       Gerard Gauthier
  Author:         J. Pitts Jarvis III (+), 3Com Corporation, October 1990
  Target Device:  Portable

  (+) very sadly, J. Pitts Jarvis III born in Fayetteville, Arkansas on November 3, 1946
      died suddenly of a heart attack on October 10, 2003, in Palo Alto, California.

  Computes CORDIC (COordinate, Rotation DIgital Computer)
  constants and exercises the basic algorithms
  Represents all numbers in fixed point notation, as a 32-bit long integer:
   1 bit sign, (31-n) bits for integral part, and n bits for fractional part
   for example, n=29 lets us represent numbers in the interval [-4, 4[ in 32 bit
   Two's complement arithmetic is operative here

  Usage: fast sine, cosine, arctg, etc

*******************************************************************************/
#ifndef CORDIC_HEADER
#define CORDIC_HEADER

#ifdef __cplusplus
extern "C" {
#endif

// define CORDIC_TEST_FCT is you want to have print test functions and checking
#define nCORDIC_TEST_FCT

//****************************************
//  include files
//****************************************
#ifdef CORDIC_TEST_FCT

#include <stdio.h>

#endif
//****************************************
//  macro definitions
//****************************************

#define cordic_longBits      32 /* never change this! */
#define cordic_fractionBits  16 /* fractional part - this one can be redefined max: 29 */
#define cordic_nonFracBits   (cordic_longBits - cordic_fractionBits)
#define cordic_One           (1L << cordic_fractionBits)
#define cordic_HalfPi        (cordic_One | 0x921fb544 >> cordic_nonFracBits)

#ifdef CORDIC_TEST_FCT

#define cordic_WriteFraction(n) cordic_fWriteFraction(n, stdout)
#define cordic_WriteRegisters cordic_fWriteRegisters(stdout)
#define cordic_WriteVar(n) cordic_fWriteVar(n, stdout)

#endif

//****************************************
//  structure definitions
//****************************************

/*
 * makes explicit mention of a long as being made
 * of a sign, a integral and a fractional part
 * not used internally by implementation
 */
typedef long cordic_Number;

//****************************************
//  global variable definitions
//****************************************

/* X, Y, Z result registers of cordic functions */
extern cordic_Number cordic_X, cordic_Y, cordic_Z;

/***************************************
 * seeds for circular and hyperbolic
 ***************************************/
/* 'K' prescaling constant for circular functions */
/*   cordic_Circular(cordic_X0C, 0, a) -> cos(a), sin(a), 0 */
extern cordic_Number cordic_X0C;
/* 'K' prescaling constant for hyperbolic functions */
/*   cordic_Hyperbolic(cordic_X0H, 0, a) -> cosh(a), sinh(a), 0 */
extern cordic_Number cordic_X0H;
/* constant useful for reciprocal hyperbolic function: (cordic_X0H/2)^2 */
/*   cordic_InvertHyperbolic(a+1, a-1, 0) -> 2*sqrt(a)/cordic_X0R, 0, ln(a)/2 */
extern cordic_Number cordic_X0R;

/* e: base of natural logarithms */
extern cordic_Number cordic_E;
/* 1/e */
extern cordic_Number cordic_OneOverE;

/* constant used in simultaneous sqrt and ln computations: -ln(cordic_X0H/2) */
/*  cordic_InvertHyperbolic(x+cordic_X0R, x-cordic_X0R, cordic_HalfLnX0R) -> sqrt(a), 0, ln(a)/2*/
extern cordic_Number cordic_HalfLnX0R;

//****************************************
//  function prototypes
//****************************************
// init
void cordic_Init(void);

// cordic functions
void cordic_Circular(cordic_Number x, cordic_Number y, cordic_Number z);
void cordic_InvertCircular(cordic_Number x, cordic_Number y, cordic_Number z);
void cordic_Hyperbolic(cordic_Number x, cordic_Number y, cordic_Number z);
void cordic_InvertHyperbolic(cordic_Number x, cordic_Number y, cordic_Number z);
void cordic_Linear(cordic_Number x, cordic_Number y, cordic_Number z);
void cordic_InvertLinear(cordic_Number x, cordic_Number y, cordic_Number z);

#ifdef CORDIC_TEST_FCT

// test
void cordic_fWriteFraction(cordic_Number n, FILE* fp);
void cordic_fWriteRegisters(FILE* fp);
void cordic_fWriteVar(cordic_Number n, FILE* fp);
void cordic_Test(void);

#endif

// low level, utilities
cordic_Number cordic_Reciprocal(unsigned n, unsigned k);
cordic_Number cordic_ScaledReciprocal(cordic_Number n, unsigned k);
long cordic_Poly2(int log, unsigned n);

#ifdef __cplusplus
}
#endif

#endif // multiple include avoidance

/******************************************************************************
  End of file
*******************************************************************************/
