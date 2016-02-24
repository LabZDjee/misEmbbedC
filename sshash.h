/**
 * \defgroup simpleStaticHashTable implementation of a simple static hash table
 * \{
 * Purpose
 * =======
 * This module implements a simple static hash table (an associative array)
 * which references custom data value items with keys\n
 * As many hash table sets can be constructed as the user wants
 * Each hash table sets is defined by:
 * - hash modulo: the number of buckets to distribute items according to their hash values
 * - max number of items that hash table set can support (in this it is static)
 * - compare and hash functions
 *
 * Compare and hash functions have to be provided by the user to allow for a more agnostic
 *
 * Details
 * =======
 *
 * Maximum number of data items in this implementation is 65,535\n
 * No assumption is made on the key structure (string, integer, ...), so the user has to define:
 * two functions to attach when constructing the hash table:
 * - function which computes the hash value of a key, given an item
 * - function to compare two keys, given two items
 *
 * These functions are actually called back by the implementation\n
 * There are two internal hash tables for a hash table set, plus a structure \ref _sshsh_HashTable\n
 * storing working values and parameters given by the construcutor\n
 * First internal table is the bucket table\n
 * Second internal table is the item reference table\n
 * Both handle _indexes_ which are <pre>unsigned short int</pre> which acts as pointers
 * to table elements. \c 0xffff indicates an index to nothing\n
 * Bucket indexes entries in the item reference table:
 * - entry (if any) index for hash value 0
 * - entry (if any) index for hash value 1
 * - ***
 * - entry (if any) index for hash value modulo-1
 *
 * Item reference table consists of interspersed link lists\n
 * This table contains strucutres \ref _sshsh_HashTableLinkElt which consist
 * of a reference to a data item and an index to the next element with the same hash value
 * if any
 * Space for internal hash tables has to be provided by the user at construction time\n
 *
 * Notes
 * =====
 * This module has no dependencies
 *
 * \file sshash.h
 * \brief header of the simple hash table module
 * \author Gerard Gauthier
 * \date 2015/06
 */
#ifndef __SS_HASH_H__
#define __SS_HASH_H__

/*
 * Hash table lookup helper for fast retreiving of data structure associated
 * with indentifying strings within the structure (or indirectly stored)
 * This helper object is given an internal buffer at construction time
 *
 * prefix sshsh_
 */

/*
 * 65,535 elements as max size
 */

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * define here qualification for pointers (such as FAR, huge...)
 */
#define POINTER_ATTR

#ifndef BOOL
 #define BOOL int
#endif

/*
 * Internal data structures used by implementation
 * Only given to compute internal hash table size needed at contruction time
 */
typedef struct _ssht_internal_tag_HashTableLinkElt
 {
   void POINTER_ATTR* mData;   /* pointer to target structure */
   unsigned short mNextLinkIdx;	/* index to next link element */
 } _sshsh_HashTableLinkElt;

typedef struct _ssht_internal_tag_HashTableEntry
 {
  unsigned short mFirstLinkIdx; /* index to first link element */
 } _sshsh_HashTableEntry;
 
/** \brief user defined function comparing 2 keys
 * defines a function which compares keys within two data structures \c s1 ans \c s2 
 * return value expected:
 *  - <0 if key within s1 is less than key within s2
 *  - zero if both keys are equal
 *  - >0 if key within s1 is less than key within s2
 */
typedef int (*sshsh_fctCmp)(void POINTER_ATTR* s1, void POINTER_ATTR* s2);
/** \brief user defined function giving hash value of a key
 * defines a function which hash value computes value of a key within data structure \c s
 * return value expected: a number between 0 and the \c modulo (as provided in \ref sshsh_ctor)
 */
typedef unsigned short(*sshsh_fctComputeHash)(void POINTER_ATTR* s, unsigned short modulo);

/*
 * member definitions for the hash table class
 */
typedef struct _ssht_internal_tag_unsigned
 {
  /* Pointer to the first element of the internal buffer: the hash table
     Each entry point has an index to a link list */
  _sshsh_HashTableEntry POINTER_ATTR* mHashTable;
  /* Pointer to the second element of the internal buffer: the link list table
     this table is composed of interspeded lists whose head index are recorded in the
     hash table and whose tails are the 'null' element at index mMaxElement */
  _sshsh_HashTableLinkElt POINTER_ATTR* mLinkTable;
  /* hash table size */
  unsigned short mHashModulo;
  /* link list table size */
  unsigned short mMaxElements;
  /* number of added elements */
  unsigned short mElementCount;
  /* first free place where a new element will be added */
  unsigned mFirstFree;
  /* call-back function for comparisons */
  sshsh_fctCmp fctCompare;
  /* call-back function for computing hash value */
  sshsh_fctComputeHash fctComputeHashValue;
 } _sshsh_HashTable;

/*
 * Macro that gives the necessary storage size of the internal table given
 * a modulo and the max number of managed elements
 */
#define HASHTABLE_SPACE_REQUIREMENT(MODULO, MAXELTS)\
  (MODULO)*sizeof(_sshsh_HashTableEntry)+(MAXELTS)*sizeof(_sshsh_HashTableLinkElt)

/*
 *  Constructor
 *
 *  modulo: hash table size. Should be roughly the average element size for best efficiency
 *  maxElts: max number of elements that can be added (max is 0xfffe: given value will be
 *           trimmed to this)
 *  hashTable: buffer to provide for static internal table and list
 *             should be modulo*sizeof(_HashTableEntry)+maxElts*sizeof(_HashTableLinkElt) large
 *  fCompare: user defined function for comparing two keys within two data structures
 *  fctComputeHashValue: user defined function for computing hash value of a key within a data structure
 */
void sshsh_ctor(_sshsh_HashTable POINTER_ATTR* This,
		  unsigned short modulo, unsigned short maxElts, void POINTER_ATTR* hashTable,
		  sshsh_fctCmp* fCompare, sshsh_fctComputeHash fctComputeHashValue);
/*
 * Static method that gives the necessary storage size of the internal table given
 * a modulo and the max number of managed elements
 * Useful for dynamic allocation
 */
unsigned long sshsh_hashTableSpaceRequirement(unsigned short modulo, unsigned short maxElts);

/*
 * Add a new record (newDatum) to the hashtable
 * If a record with the same key already exists, it is replaced
 * Return false is no room is available and nothing is added then
 */
BOOL sshsh_add(_sshsh_HashTable POINTER_ATTR* This, void POINTER_ATTR* newDatum);

/*
 * Remove a record corresponding to key from the hashtable
 * Return false if record is not found
 */
BOOL sshsh_remove(_sshsh_HashTable POINTER_ATTR* This, const char POINTER_ATTR* key);

/*
 * Empty hash table
 */
void sshsh_removeAll(_sshsh_HashTable POINTER_ATTR* This);

/*
 * Retrieve a record given a key
 * Return NULL if not found
 */
void POINTER_ATTR* sshsh_lookup(_sshsh_HashTable POINTER_ATTR* This, const char POINTER_ATTR* key);

/*
 * Iterator. Note: record are retrieved in any order
 * Return the next record given a pointer to a position
 * First record is reached by setting *pPosition to -1
 * When trying to reach one record after the last one, NULL is returned (and
 * *pPosition is set back to -1)
 */
void POINTER_ATTR* sshsh_iter(_sshsh_HashTable POINTER_ATTR* This, long* pPosition);

/*
 * Iterator retrieving records sorted by key
 * Return the next record given a pointer to a position
 * First record is reached by setting *pPosition to -1
 * When trying to reach one record after the last one, NULL is returned (and
 * *pPosition is set back to -1)
 * Note: this method is not optimised for speed (o2)
 */
void POINTER_ATTR* sshsh_sortedIter(_sshsh_HashTable POINTER_ATTR* This, long* pPosition);

/* Low level stuff (given for debug purpose only) */

/*
 * Gives the hash value given a key 
 * This value lies within modulo range (0; modulo-1)
 * maxStrLg is the max key length
 */
unsigned short _sshsh_computeHValue(const char POINTER_ATTR* key, unsigned short modulo, unsigned short maxStrLg);

#ifdef  __cplusplus
 } // extern C
#endif

/**
 * \}
 */

#endif
