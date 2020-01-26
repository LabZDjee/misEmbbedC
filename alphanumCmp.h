#ifndef __ALPHANUM_CMP_H_INC__
#define __ALPHANUM_CMP_H_INC__
/**
 * \defgroup alphanumericCmp extended alphanumeric comparison
 * \{
 * Purpose
 * =======
 * This module, \c alphanumericCmp, defines an extended string comparison utility whose primary role\n
 * is to allow alphanumeric comparisons: "Foo128" > "Foo12"
 *
 * Details
 * =======
 * This module implements one main function \ref ANCnsStrCmp which compares two stringZ C strings\n
 * It takes a composed parameter: structure \ref ANCns_sProfile\n
 * This parameter structure allows to define if comparison has to be case sensitive, maximum length
 * of strings (if comparison should stop before meeting a \c '\0' in case this length is reached),\n
 * and definition of spaces (if spaces have to be considered in the comparison)\n
 * Comparison is alphanumeric: the main routine seeks subgroups of characters, digits, and spaces\n
 * and compares within matching subgroups or if subgroups don't match considering the following hierarchy:\n
 * - subgroup of characters > subgroup of digits > subgroup of spaces
 *
 * About spaces:
 * - subgroup of spaces are considered equal no matter what they are made of
 * - if no space is defined, even natural spaces like <tt>' '</tt> are considered characters
 *
 * Case sensitivity only affects character comparison, not definition of spaces
 *
 * Here are some examples with case sensitive flag turned off, no max length considered,\n
 * and space is defined as " \t":
 * - "abc" < "abd"
 * - "abc" < "abc1"
 * - "ab110" > "ab19"
 * - "ab 110" < "ab19"
 * - "ab__110" > "ab\t19"
 * - "abc " < "abc1"
 * - "ab cd 008" = "ab__cd__8"
 * - "ab cd 008" < "ab__cd__8b"
 *
 * Notes
 * =====
 * This module has no dependencies to remain simple and portable: testing if a character is a digit\n
 * making upper case, etc are all locally defined\n
 * All the low-level routines defined for the implementation are exposed\n
 *
 * In order to make comparisons, this module relies on internal integer indexes (as can be seen with low\n
 * level routine \def ANCnsGetValue as an example). Those indexes are offsets to C strings and
 * their type is defined as custom type \ref ANCnsStrSz. <tt>unsigned short</tt> seems a reasonable\n
 * choice for this type although it could be narrowed to <tt>unsigned char</tt> or widen to\
 * <tt>unsigned long</tt>\n
 *
 * \file alphanumericCmp.h
 * \brief header of the alphanumericCmp module
 * \author Gerard Gauthier
 * \date 2015/05
 */

/** \brief typedef which defines the maximum offset to a string
 * normally unsigned short should fit
 * it is the equivalent of \c size_t in standard library
 * \warning if definition is altered here, the module *should be recompiled*
 */
typedef unsigned short ANCnsStrSize;

/** \brief profile: structure for comparison parameters
 *
 * this structure gathers parameters for string comparisons implemented in this module
 * notes:
 *  - if no maximum length defined (-1) comparison stops at the first '\0' character found
 *  - if a '\0' is met before maximum length, comparison will also stop at this point
 *  - as mentioned, case sensitivity does not apply to the definition of spaces
 */
typedef struct ANCns_sProfile
{
 unsigned char bCaseInsensitive;  /**< non-zero if comparison for normal characters (not space) should be case insensitive */
 int maxLength;  /**< max. length of the comparison - -1 for no max length */
 const char* pSpaceDef; /**< stringZ defining spaces - NULL if not looking for spaces */
} ANCnsSProfile;

/** \brief **alphanumeric comparison of two C strings**
 *
 * extended comparison function, similar to \c string.h::strcmp:
 * - splits strings into: characters, decimal digits, and optional user-defined spaces
 * - characters being defined as non spaces and non decimal digits ('0' to '9')
 * - can compare strings considering or ignoring case of characters
 * - searches for decimal numeric groups within strings to compare and compare their values
 * - can consider groups of user-defined spaces as a single space
 * - can limit the number of characters in the comparison (similar to \c string.h::strncmp)
 *
 * in the comparison, characters have a superior rank than digits, which have a superior rank than spaces\n
 *
 * if \p profile is set to \c NULL, then a default profile will be used, with the following definition
 * - case sensitive compare
 * - no length limit on strings
 * - no space defined
 *
 * \return
 * - negative if \p s2 is greater then \p s1 (e.g.: s1="bar", s2="foo")
 * - zero if both strings are equal
 * - positive if \p s1 is greater then \p s2 (e.g.: s1="zoo", s2="boo")
 */
int ANCnsStrCmp(const char* s1 /**<  first string to compare */,
                const char* s2 /**<  second string to compare */,
                const ANCnsSProfile* profile /**< reference to comparison parameters - can be NULL */);

/** \brief Low level: **tests if a character is a decimal digit**
 *
 * checks whether character is in the '0' to '9' range
 *
 * \return non-zero if character is a decimal digit, 0 if it is not
 */
int ANCnsIsDigit(char c /**< character under test */);

/** \brief Low level: **extracts a value from string of decimal characters**
 *
 * this value is extracted from digits at \p str, offset referenced by \p pOffset
 * \p maxLength gives the maximum length of str (not taking the offset into account)
 * if -1, str + offset is scanned up to the first '\0'\n
 * if a '\0' is met before maxLength when defined, scan will stop\n
 * \p pOffset is updated to reference the updated offset after the scan:
 *  - first character met which was not a digit
 *  - offset equaled maxLength
 *
 * so, if value referenced by \p pOffset didn't change, it means scan was unsuccessful
 * \p pOffset can be NULL: in this case a zero offset is considered and there is no way to know
 * where the scan stopped and thus if any valid digit was found
 *
 * \return extracted value (0 in case of failure)
 */
unsigned long ANCnsGetValue(const char* str /**< string to test */,
                            int maxLength /**< max. length of the comparison - -1 for no max length */,
                            ANCnsStrSize* pOffset /**< in/out: offset within str - can be NULL */);

/** \brief Low level: **get the upper case value of a given character**
 *
 * if character is in the 'a' to 'z' range, make the result in the 'A' to 'Z' range\n
 * otherwise does nothing
 *
 * \return upper-case value of \p c
 */
char ANCnsToUpper(char c /**< reference character */);

/** \brief Low level: **tests if a character is within a list**
 *
 * Verifies whether a character \p c can be found within a \p list (a reference
 * to a C stringZ which enumerates the candidate characters)
 * \return
 *  - -1 if \p c is not found in \p list (or if \p list is NULL)
 *  - zero-based offset of position of \p c within \p list
 */
int ANCnsStrChr(char c /**< character under test */,
                const char* list /**< list of characters to look into */);

/** \brief Low level: **Extracts a string entity (no value-no space, value, spaces)**
 *
 * Extracts a string entity at a certain (zero-based) offset\n
 * A string entity can be:
 *  - 1/ a contiguous series of spaces
 *  - 2/ a contiguous series of decimal digits
 *  - 3/ a contiguous series of characters which are neither decimal digits nor spaces
 *
 * Spaces are defined by \p profile: its field \c pSpaceDef references a string which
 * defines what a space is\n
 * The extraction starts at offset references by \p pOffset\n
 * \p pOffset is increased by the number of characters of the found entity\n
 * \p pOffset can be \c NULL; in this case offset is supposed to be zero and no information
 * of the entity length is returned\n
 * \p pResult if non null returns some additional information about the entity\n
 * parameter \c maxLength reference by \p profile is also taken into account to not
 * search beyond this limit as well as the \c '\0' termination character in regular strZ C strings\n
 * Depending on the type of entity as described above, result can be:
 *  - number of contiguous spaces found
 *  - value the decimal digits represent
 *  - value of first char, shifted to upper case if parameter \c bCaseInsensitive is set
 *
 * \return
 *  - 0: in case of error (NULL pointers, empty string at offset
 *  - 1: space(s) at offset
 *  - 2: numeric value at offset
 *  - 3: characters (non-numeric and non-space token) at offset
 */
int ANCnsExtract(const char* str /**< reference to string to search into */,
                 const ANCnsSProfile* profile /**< reference parameters */,
                 unsigned long* pResult /**< out: reference to result */,
                 ANCnsStrSize* pOffset /**< in/out: offset within \p str */);

/** \brief **fast alphanumeric comparison of two C strings**
 *
 * fast alphanumeric comparison function, similar to \c string.h::strncmp:
 * - splits strings into: characters and decimal digits
 * - characters being defined as non spaces and non decimal digits ('0' to '9')
 * - compares strings considering case of characters
 * - searches for decimal numeric groups within strings to compare and compare their values
 * - limits the number of characters in the comparison (similar to \c string.h::strncmp)
 *
 * in the comparison, characters have a superior rank than digits\n
 *
 * \return
 * - negative if \p s2 is greater then \p s1 (e.g.: s1="bar", s2="foo")
 * - zero if both strings are equal
 * - positive if \p s1 is greater then \p s2 (e.g.: s1="zoo", s2="boo")
 */
int ANCnsFastStrncmp(const char* s1 /**< first string to compare */,
                     const char* s2 /**< second string to compare */,
                     ANCnsStrSize maxLength /**< max length of comparison */);

/**
 * \}
 */

#endif /* __ALPHANUM_CMP_H_INC__ */
