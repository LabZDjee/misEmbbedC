#ifndef __JEEP_DEFINED__
#define __JEEP_DEFINED__

/**
 * \defgroup JEEP General Purpose Pure Functions
 * \{
 * This module is implements misc pure functions
 *
 * \file gtimer.h
 * \brief header of the gTimer module
 * \author Gerard Gauthier
 * \date 2016/02
 */
 
 /*
  * dependencies
  */
 
 /* where boolean, dword, word, byte... should be defined as types */
 #include "terms.h"

 /** \brief **gets some bits in a dword**
 *
 * \return extracted bit, right justified
 *
 * \details this function returns width bits of dword value
 * from a zero-based bit offset
 * examples:
 *  - (0x1f04, 8, 4) => 0xf0
 *  - (0x12345678, 7, 11) => 0xa
 *	- (0x87654321, 11, 11) => 0x4a8
 *	- (0x12345678, 4, 0) => 8
 *	- (0x12345678, 8, 24) => 0x12
 *	- (0x12345678, 12, 25) => 9
*	- (0x12345678, 16, 15) => 0x2468
 */
dword getBitsInDword(dword dw /**<  value */, 
                     byte width /**<  number of bits to get */,
					 byte bitOffset /**< zero-based offset of first bit */);

 /** \brief **positions some bits in a dword**
 *
 * \return dword with bits positioned in it
 *
 * \details this function returns width bits of dword value
 * from a zero-based bit offset
 * examples:
 * - (0x12345678, 0xaa, 8, 16) => 0x12aa5678
 * - (0xaaaaaaaa, 0x5555, 12, 4) => 0xaaaa555a
 * - (0xffffffff, 0, 9, 13) => 0xffc01fff
 */
dword setBitsInDword(dword dw /**<  reference dword to work on */,
                     dword value /**< value of bits to position */,
					 byte width /**< number of bits to position */,
					 byte bitOffset /**<  zero-based offset of first bit */);

 /**
 *  \brief **in-place find and replace string**
 *  
 *  \return number of replacements made
 *
 *  \details replace old string to new string  in a string for a maximum
 *  number of occurences given as a parameter
 *  if the given maximum number of occurences is less than 1, it means replace
 *  all the occurences
 *  examples:
 *   ("kludge of kludge, whole kludge", "kludge", "cruft", 2) => 2 and
 *     first parameter altered into "cruft of cruft, whole kludge"
 *   ("to be or not to be", "be", "2-b", 0) => 2
 *  
 *  \warning
 *  - make sure str has enough room (no control is made on this)
 *  - no control is done on nullity of any of pointers
 *  \note in order to precompute room for replacement, in case newS is
 *  longer than oldS, a 'fake' call to this function can be made
 *  with oldS == newS, to estimate the extra room required
 */
 int multipleFindAndReplace(char *str /**< [in/out] where replacements should take place */,
                            const char* oldS /**< old string */, 
							const char* newS /**< new string */, 
							int maxOcc /**< max number of occurences */);
 
 /**
 *  \brief converts ASCII char input as an hexadecimal digit 
 *  
 *  \return 0 to 15 if conversion ok or 255 if input wrong
 *  
 *  \details ASCII char input has to hexadecimal digit: [0-9A-Fa-f]
 */
 byte ascii1byte (byte Ascii /**< character to convert */);

 
/**
 * \}
 */
#endif
