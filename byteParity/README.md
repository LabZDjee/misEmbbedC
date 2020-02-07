# Calculate [Parity Bit](https://en.wikipedia.org/wiki/Parity_bit) of a Byte

Calculates the imparity of a 8-bit byte (octet), that is, estimates whether number of bits set to one in this byte is an odd number

Reasonably quick because of a 16-byte look-up table which splits byte into two 4-bit nibbles, so a compromise between speed and memory occupation

## API

### `byteOddParityInit`

Should be called before any parity calculation
Initializes look-up table in RAM memory

### `int calcByteOddParity(unsigned char byte)`

Checks if parity 'byte' is *odd*
Returns 1 if parity is odd and 0 if it is even

### `CALC_BYTE_ODD_PARITY(byte)`

Macro which basically *inlines* `calcByteOddParity`

Parameter `byte` is used two times, beware of possible side effects

### `unsigned char byteWithParity(unsigned char byte, int parityType)`

Add parity bit of a 7-bit byte by setting its most significant bit to zero or one as necessary

Depending on `parityType`:

- `1`: return byte with parity set to odd
- `2`: return byte with parity set to even
- `0`: simply return the byte with most significant bit set to 0