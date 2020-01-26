/******************************************************************************
  FILENAME: FractMath.h

 *****************************************************************************
 *****************************************************************************

  Author:         Gerard Gauthier
  Target Device:  Portable code - not threadsafe

  Note: general purpose maths on binary fractional 32-bit numbers
        this module is purely functional (stateless)
        to the exception of qn_fractionBits
        which is a reference for the fractional split
        These fractional numbers are sometimes referred to as Qi.f
        i, being the number of bits for the integral part and f, being
        the number of bits for the fractional part (i+f=32)
        Here f is represented by the global variable qn_fractionBits which
        can dynamically change

*******************************************************************************/
#ifndef FRACTMATH_HEADER
#define FRACTMATH_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//****************************************
//  include files
//****************************************

//****************************************
//  macro definitions
//****************************************
/*
 * Some useful math around fractional numbers
 */
/* Quick multiply by five. Beware: cn is expanded more than one time */
#define qn_MULT5(cn)  ((cn << 2) + cn)
/* Quick multiply by ten. Beware: cn is expanded more than one time */
#define qn_MULT10(cn) (qn_MULT5(cn) << 1)

/*
 * Maximum number of fractional bits
 * Minimum is zero (a pure long integer)
 */
#define qn_MAXFRACTBITS         30

/*
 * Default number of fractional bits
 * Value taken at startup
 */
#define qn_DEFFRACTBITS         17

/*
 * max number of decimals for formated display in decimal
 * see qn_SPrintFDecimalNumber for their usage
 * qn_FORMAT_MAX_POS_DEC: max decimals right to fractional point
 * qn_FORMAT_MAX_NEG_DEC: max decimals zeroed left to fractional point
 */
#define qn_FORMAT_MAX_POS_DEC    8
#define qn_FORMAT_MAX_NEG_DEC    9

/*
 * Formalized results for some operations
 */
#define qn_RES_OK        0 /* operation done */
#define qn_RES_POVERFLOW 1 /* positive overflow */
#define qn_RES_NOVERFLOW 2 /* negative overflow */
#define qn_RES_PINF      3 /* positive infinity */
#define qn_RES_NINF      4 /* negative infinity */
#define qn_RES_UNDEF     5 /* undefined (e.g. 0/0) */

/*
 * Operations qn_ToLong function can perform
 */
#define TOLONG_OP_FLOOR        1 /* floor(1.x) = 1, floor(-1.x)=-2 (x!=0) */
#define TOLONG_OP_CEIL         2 /* ceil(1.x) = 2, ceil(-1.x)=-1 (x!=0) */
#define TOLONG_OP_ROUND        3 /* round(1.9) = 2, round(-1.9)=-2, round(1.4) = 1, round(-1.4)=-1 */
#define TOLONG_OP_TRUNC        4 /* trunc(1.x) = 1, trunc(-1.x)=-1 */
#define TOLONG_OP_EXCESS       5 /* excess(1.x) = 2, excess(-1.x)=-2 (x!=0) */

//****************************************
//  structure definitions
//****************************************
/*
 * The fractional number type
 * Represents all numbers in fixed point notation, as a 32-bit long integer:
 *  1 bit sign, (31-n) bits for integral part, and n bits for fractional part
 *  for example, n=16 represents numbers in the interval [-32,768, 32,768[
 * More generally the interval is this: [-2^(31-n), 2^(31-n)[
 * Beware, when very close to 2^(31-n): rounding can trigger overflows
 * Two's complement arithmetic is operative here
 */
typedef long qn_Number;

//****************************************
//  global variable definitions
//****************************************
/*
 * The fractional part in terms of number of bits
 * Don't change this value, call 'qn_Init' instead
 */
extern unsigned char qn_fractionBits;

//****************************************
//  function prototypes
//****************************************
void qn_Init(unsigned char nbFractionalBits);
short qn_ReadDecimalNumber(const char* pStr, qn_Number* pResult);
short qn_SPrintFDecimalNumber(qn_Number value, char* pResult, signed char nbDec, unsigned char nbWidth, unsigned char bThousandSep);
qn_Number qn_Mul(qn_Number a, qn_Number b, unsigned char* pErr);
qn_Number qn_Div(qn_Number a, qn_Number b, unsigned char* pErr);
signed char qn_Sign(qn_Number qn);
qn_Number qn_Cnv(qn_Number qn, unsigned char presentNbofFractionalDigits,
                 unsigned char nextNbofFractionalDigits, unsigned char* pErr);
long qn_ToLong(qn_Number qn, unsigned char op);

unsigned char qn_RankOfHigherBit(unsigned long value);
unsigned char qn_RankOfLowerBit(unsigned long value);

#ifdef __cplusplus
}
#endif

#endif // multiple include avoidance

/******************************************************************************
  End of file
*******************************************************************************/
