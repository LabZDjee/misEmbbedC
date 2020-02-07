#ifndef __BYTE_PARITY_H__INCLUDED__
#define __BYTE_PARITY_H__INCLUDED__

/*
 * Calculate parity of a byte
 * Reasonably quick because of a 16-byte look-up table
 */

/* internal lookup table (exposed for macro) */
extern char __nibbleParity_LUT__[16];

/*
 Should be called before any parity calculation
 Initializes look-up table
*/
void byteOddParityInit();

/*
 Macro for checking if parity of an unsigned char (8-bit) is odd
  => 1 for yes, 0 for no
 Note: parameter 'byte' is used two times, beware of possible side effects
*/
#define CALC_BYTE_ODD_PARITY(byte) (__nibbleParity_LUT__[(byte) & 15] ^ __nibbleParity_LUT__[(byte) >> 4])

/*
 Checks if parity 'byte' is odd
 Returns 1 if parity is odd and 0 if it is even
*/
int calcByteOddParity(unsigned char byte);

/*
 Add parity bit of a 7-bit byte by setting its eight-bit to zero or one as necessary

 Depending on parityType:
  - 1: return byte with parity set to odd
  - 2: return byte with parity set to even
  - 0: simply return the byte with most significant bit set to 0
*/
unsigned char byteWithParity(unsigned char byte, int parityType);

#endif
