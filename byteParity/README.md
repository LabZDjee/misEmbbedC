# Calculate [Parity Bit](https://en.wikipedia.org/wiki/Parity_bit) of a Byte

Reasonably quick because of a 16-byte look-up table which 

## API

`byteOddParityInit`

Should be called before any parity calculation
Initializes look-up table

`int calcByteOddParity(unsigned char byte)`

Checks if parity 'byte' is odd
Returns 1 if parity is odd and 0 if it is even

`CALC_BYTE_ODD_PARITY(byte)`

Macro which basically *inlines* `calcByteOddParity`

Parameter `byte` is used two times, beware of possible side effects

