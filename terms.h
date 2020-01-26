#ifndef __TERMS_H_INCLUDED__
#define __TERMS_H_INCLUDED__
/**
 * \file terms.h
 * \brief defines handy useful definitions that C language lacks
 *
 */

#ifndef FALSE
 #define FALSE (0) /* boolean false*/
#endif // FALSE

#ifndef TRUE
 #define TRUE (!FALSE) /* boolean true */
#endif // TRUE

#ifndef NULL
 #define NULL ((void*)0) /* null pointer */
#endif // NULL

typedef unsigned char boolean;   /**< should contain TRUE or FALSE */
typedef unsigned char byte;   /**< shorthand for 8-bit unsigned */
typedef unsigned short word;  /**< shorthand for 16-bit unsigned */
typedef unsigned long dword;   /**< shorthand for 32-bit unsigned */
typedef signed char schar; /**< shorthand for 8-bit signed */
typedef short int sint; /**< alias for short int */

/** \def UNUSED_FCT_P
 *  \brief unused function parameter
 */
#ifdef __GNUC__ /* gcc */
 #define UNUSED_FCT_P __attribute__((__unused__))
#else
 #define UNUSED_FCT_P /* not handled */
#endif // __GNUC__

#endif // __TERMS_H_INCLUDED__
