/*
--------------------------------------------------------------------------------
	Class        : Static string hash table implementation file (Prefix : SSHT_)
	Author       : G. Gauthier
	Company      : SAFT - Tours 
	Description  : This class implements a static hash table for fast
	               retreiving of data structures associated with
				   indentifying strings within the structure
--------------------------------------------------------------------------------
*/
#include "sshash.h"

#ifndef NULL
 #define NULL ((void POINTER_ATTR*)0)
#endif


/*
 *------------------------------------------------------------------------------
 * Gives the hask value given a key
 * This value lies within modulo range (0; modulo-1)
 *------------------------------------------------------------------------------
 */
ushort _SSHT_computeHValue(const char POINTER_ATTR* key, ushort modulo, ushort maxStrLg)
 {
  ushort acc;
  ushort i;
 
  for(i=0, acc=0; i<maxStrLg && key[i]!='\0'; i++)
   if(i%2)
    acc += (ushort)key[i];
   else
	acc += ((ushort)key[i] << 8);
  return(acc%modulo);
 }

/*
 *------------------------------------------------------------------------------
 * Given a key, does a lookup to find a target structure
 * Returns a pointer to the target structure if found or NULL if not found
 * Whatever the returned value,*pHashTableEntry entry is filled with the
 * calculated hash table index and *pHashTableLinkElt is filled with the
 * index to the associated link element, either associated with the found element
 * or the last element index in the link list or mMaxElements if the link list
 * is empty
 *------------------------------------------------------------------------------
 */
void POINTER_ATTR* _SSHT_internalLookup(const StaticStrHashTable POINTER_ATTR* This,
                                        const char POINTER_ATTR* key,
										ushort* pHashTableEntry,
										ushort* pHashTableLinkElt)
 {
  char POINTER_ATTR* pKeyInData;
  ushort nextElt;
  *pHashTableEntry=_SSHT_computeHValue(key, This->mHashModulo, This->mMaxKeySize);
  *pHashTableLinkElt=This->mHashTable[*pHashTableEntry].mFirstLinkIdx;

  while(*pHashTableLinkElt<This->mMaxElements)
   {
	pKeyInData=(char POINTER_ATTR*)This->mLinkTable[*pHashTableLinkElt].mData + This->mKeyOffset;
    if(This->mbIndirect)
     pKeyInData=(char POINTER_ATTR*)*(char POINTER_ATTR* POINTER_ATTR*)pKeyInData;
	if(!strncmp(key, pKeyInData, This->mMaxKeySize))
	 return((void POINTER_ATTR*)This->mLinkTable[*pHashTableLinkElt].mData);
	if((nextElt=This->mLinkTable[*pHashTableLinkElt].mNextLinkIdx)>=This->mMaxElements)
	 break;
    *pHashTableLinkElt=nextElt;
   }
  return(NULL);
 }

/*
 *------------------------------------------------------------------------------
 */
void SSHT_ctor(StaticStrHashTable POINTER_ATTR* This,
		  ushort modulo, ushort maxElts, void POINTER_ATTR* hashTable,
		  ushort keyOffset, BOOL bIndirect, ushort MaxKeySize)
 {
  This->mbIndirect=bIndirect;
  This->mKeyOffset=keyOffset;
  This->mHashModulo=modulo;
  This->mHashTable=(_SSHT_HashTableEntry POINTER_ATTR*)hashTable;
  This->mLinkTable=(_SSHT_HashTableLinkElt POINTER_ATTR*)((_SSHT_HashTableEntry POINTER_ATTR*)hashTable+modulo);
  This->mKeyOffset=keyOffset;
  This->mMaxKeySize=MaxKeySize;
  if(maxElts>0xfffe)
   maxElts=0xfffe;
  This->mMaxElements=maxElts;
  SSHT_removeAll(This);
 }

/*
 *------------------------------------------------------------------------------
 */
ulong SSHT_hashTableSpaceRequirement(ushort modulo, ushort maxElts)
 {
  if(maxElts>0xfffe)
   maxElts=0xfffe;
  return(HASHTABLE_SPACE_REQUIREMENT(modulo, maxElts));
 }

/*
 *------------------------------------------------------------------------------
 */
void SSHT_removeAll(StaticStrHashTable POINTER_ATTR* This)
 {
  long i;
  for(i=0; i<This->mHashModulo; i++)
   This->mHashTable[(ushort)i].mFirstLinkIdx=This->mMaxElements;
  This->mElementCount=0;
  This->mFirstFree=0;
  for(i=0; i<This->mMaxElements; i++)
   This->mLinkTable[i].mData=NULL; /* mData is NULL to indicate a free place */
 }

/*
 *------------------------------------------------------------------------------
 */
void POINTER_ATTR* SSHT_lookup(StaticStrHashTable POINTER_ATTR* This, const char POINTER_ATTR* key)
 {
  ushort HashTableEntry;
  ushort HashTableLinkElt;
  return(_SSHT_internalLookup(This, key, &HashTableEntry, &HashTableLinkElt));
 }

/*
 *------------------------------------------------------------------------------
 */
BOOL SSHT_add(StaticStrHashTable POINTER_ATTR* This, void POINTER_ATTR* pNewDatum)
{
    ushort HashTableEntry;
    ushort HashTableLinkElt;
    const char POINTER_ATTR* key;
    void POINTER_ATTR* pTarget;

    if(pNewDatum==NULL)
        return(FALSE);
    /* searches the key inside the data structure */
    key=(char POINTER_ATTR*)pNewDatum + This->mKeyOffset;
    if(This->mbIndirect)
        key=(char POINTER_ATTR*)*(char POINTER_ATTR* POINTER_ATTR*)key;
    if((pTarget=_SSHT_internalLookup(This, key, &HashTableEntry, &HashTableLinkElt))==NULL)
        if(This->mElementCount>=This->mMaxElements) /* too many elements */
            return(FALSE);
        else {
	        if(HashTableLinkElt>=This->mMaxElements) /* hash table entry pointed to 'null' */
	            This->mHashTable[HashTableEntry].mFirstLinkIdx=This->mFirstFree;
	        else /* HashTableLinkElt indexes last link element in list: hooks new one to it */
	            This->mLinkTable[HashTableLinkElt].mNextLinkIdx=This->mFirstFree;
	        /* updates new link element members: end of link list */
            This->mLinkTable[This->mFirstFree].mNextLinkIdx=This->mMaxElements;
	        This->mLinkTable[This->mFirstFree].mData=pNewDatum;
	        This->mElementCount++;	/* link list table increases */
	        /* look for next free space */
	        for(This->mFirstFree=0; This->mFirstFree<This->mMaxElements; This->mFirstFree++)
	            if(This->mLinkTable[This->mFirstFree].mData==NULL)
	                break;
	    }
    else
        This->mLinkTable[HashTableLinkElt].mData=pNewDatum; /* replace existing */
    return(TRUE);
}

/*
 *------------------------------------------------------------------------------
 */
BOOL SSHT_remove(StaticStrHashTable POINTER_ATTR* This, const char POINTER_ATTR* key)
 {
  ushort HashTableEntry;
  ushort HashTableLinkElt;
  void POINTER_ATTR* pTarget
          = _SSHT_internalLookup(This, key, &HashTableEntry, &HashTableLinkElt);
  ushort hitNext, i;

  if(pTarget==NULL)	/* not found */
   return(FALSE);
  /* stores link to next to removed element in the link list */
  hitNext=This->mLinkTable[HashTableLinkElt].mNextLinkIdx;
  /* place is freed */
  This->mLinkTable[HashTableLinkElt].mData=NULL;
  /* update first place, if needed */
  if(This->mFirstFree>HashTableLinkElt)
   This->mFirstFree=HashTableLinkElt;
  /* looks for link to removed item */
  for(i=This->mHashTable[HashTableEntry].mFirstLinkIdx;
      This->mLinkTable[i].mData!=NULL && i<This->mMaxElements ;
	  i=This->mLinkTable[i].mNextLinkIdx)
   if(This->mLinkTable[i].mNextLinkIdx==HashTableLinkElt)
    {
	 This->mLinkTable[i].mNextLinkIdx=hitNext; /* updates link */
	 break;
	}   
  /* element to remove is first in the link list? */
  if(This->mHashTable[HashTableEntry].mFirstLinkIdx==HashTableLinkElt)
   This->mHashTable[HashTableEntry].mFirstLinkIdx=hitNext;
  This->mElementCount--; /* shrink link table */
  return(TRUE);
 }

/*
 *------------------------------------------------------------------------------
 * Iterator. Note: record are retrieved in any order
 * Return the next record given a pointer to a position
 * First record is reached by setting *pPosition to -1
 * When trying to reach one record after the last one, NULL is returned (and
 * *pPosition is set back to -1)
 *------------------------------------------------------------------------------
 */
void POINTER_ATTR* SSHT_iter(StaticStrHashTable POINTER_ATTR* This, long* pPosition)
 {
  void POINTER_ATTR* pResult;
  ushort pos=(ushort)*pPosition+1;

  if(*pPosition<0)
   pos=0;
  for(; pos<This->mMaxElements; pos++)
   {
    if((pResult=This->mLinkTable[pos].mData)!=NULL)
     {
	  *pPosition=pos;
	  return(pResult);
	 }
   }
  *pPosition=-1;
  return(NULL);
 }

/*
 *------------------------------------------------------------------------------
 */
void POINTER_ATTR* SSHT_sortedIter(StaticStrHashTable POINTER_ATTR* This, long* pPosition)
 {
  ushort pos;
  ushort posMin = 0;				       /*-- just to avoid a warning --*/
  char POINTER_ATTR* pKeyInData; 
  char POINTER_ATTR* pKeyInDataMin = NULL; /*-- just to avoid a warning --*/
  char POINTER_ATTR* pKeyInDataBottom;
  char POINTER_ATTR* pCur, * pResult;

  if(*pPosition>=0 && (ushort)*pPosition<=This->mMaxElements)
  {
    pKeyInDataBottom=(char POINTER_ATTR*)This->mLinkTable[(ushort)*pPosition].mData + This->mKeyOffset;
    if(This->mbIndirect)
     pKeyInDataBottom=(char POINTER_ATTR*)*(char POINTER_ATTR* POINTER_ATTR*)pKeyInDataBottom;
   }
  else
   pKeyInDataBottom=NULL;
  for(pos=0, pResult=NULL; pos<This->mMaxElements; pos++)
   {
    if((pCur=This->mLinkTable[pos].mData)!=NULL)
     {
      pKeyInData=(char POINTER_ATTR*)pCur + This->mKeyOffset;
      if(This->mbIndirect)
       pKeyInData=(char POINTER_ATTR*)*(char POINTER_ATTR* POINTER_ATTR*)pKeyInData;
	  if(pKeyInDataBottom==NULL || strncmp(pKeyInDataBottom, pKeyInData, This->mMaxKeySize)<0)
	   if(pResult==NULL || strncmp(pKeyInData, pKeyInDataMin, This->mMaxKeySize)<0)
	    {
		 pKeyInDataMin=pKeyInData;
		 pResult=pCur;
		 posMin=pos;
		}
	 }
   }
  if(pResult!=NULL)
   {
    *pPosition=(long)posMin;
    return(pResult);
   }
  *pPosition=-1;
  return(NULL);
 }
