 /******************************************************************************
  FILENAME: FractMath.c
 *****************************************************************************

  Author:         Gerard Gauthier
  Target Device:  Portable - not thread safe

  Note: general purpose maths on binary fractional 32-bit numbers
        this module is purely functional (stateless)
        to the exception of qn_fractionBits
        which is a reference for the fractional split
        These fractional numbers are sometimes referred to as Qi.f
        i, being the number of bits for the integral part and f, being
        the number of bits for the fractional part (i+f=32)
        Here f is represented by the global variable qn_fractionBits which
        can dynamically change

  Change Log is at end of file

  Note: general purpose methods

*******************************************************************************/

//****************************************
//  include files
//****************************************

#include <string.h>

#define DEBUG

#ifdef DEBUG
 #ifdef _MSC_VER
  #include <windows.h> /* DEBUG */
 #endif
 #include <stdio.h> /* DEBUG */
#endif

#include "FractMath.h"

//****************************************
//  global variable declarations 
//****************************************
/* 
 * The fractional part in terms of number of bits
 */
unsigned char qn_fractionBits=qn_DEFFRACTBITS;

#define POS_FRACT_RSHIFT       (31)
#define MAX_POS_FRACT          (1UL<<POS_FRACT_RSHIFT)
#define NEG_FRACT_LSHIFT       (1)
#define NB_OF_MULTIPLES_OF_TEN (9)

//****************************************
//  local variable declarations 
//****************************************
static const qn_Number formatDecimalPos[qn_FORMAT_MAX_POS_DEC+1] = {
    MAX_POS_FRACT,
    (MAX_POS_FRACT+5UL)/10UL,
    (MAX_POS_FRACT+50UL)/100UL,
    (MAX_POS_FRACT+500UL)/1000UL,
    (MAX_POS_FRACT+5000UL)/10000UL,
    (MAX_POS_FRACT+50000UL)/100000UL,
    (MAX_POS_FRACT+500000UL)/1000000UL,
    (MAX_POS_FRACT+5000000UL)/10000000UL,
    (MAX_POS_FRACT+50000000UL)/100000000UL};
static const qn_Number powersOfTen[10] = {
    1UL,
    10UL,
    100UL,
    1000UL,
    10000UL,
    100000UL,
    1000000UL,
    10000000UL,
    100000000UL,
    1000000000UL};

/*****************************************************************************
    FUNCTION    qn_Init

    Description:
      Init module

    Entrance Conditions:
            
    Exit Conditions:
     qn_fractionBits properly set (between 0 and 30)      
   
    args:
      nbFractionalBits: fractional part in terms of number of bits defining
              init can be dynamically called though this is not threat safe
      
    returns:
 *****************************************************************************/
void qn_Init(unsigned char nbFractionalBits)
{
 if(nbFractionalBits>=qn_MAXFRACTBITS)
  nbFractionalBits=qn_MAXFRACTBITS;
 qn_fractionBits=nbFractionalBits;
}

/*****************************************************************************
    FUNCTION    qn_ReadDecimalNumber

    Description:
      scan a string looking for a decimal value in base 10 and store result in a fractional number
      format: [+|-]<digits>[.<digits>]
              <digits> a list of 0 to any number of 0|1|2|3|4|5|6|7|8|9
              [] denotes an option, | denotes 'or'
      leading spaces are ignored
      conversion fails if no valid character met, no digit met, or integral part overflows

    Entrance Conditions:
     qn_Init called
            
    Exit Conditions:
      
    args:
      pStr string to convert
      pResult address where to put result
              if NULL only calculates the number of digits involved in scan
               this method is much quicker than the full conversion itself
              if conversion fails, *pResult is not altered 
      
    returns:
      number of characters read from string scanned for conversion
      conversion stops at first invalid character with repect to format above
      return 0 if no valid character encountered
      return -1 if no digit encountered
      return -2 if conversion overflowed
 *****************************************************************************/
short qn_ReadDecimalNumber(const char* pStr, qn_Number* pResult)
 {
  unsigned char isPos=1, /* is result positive */
                bDone=0, /* job done */
                bAtLeastOneDigit=0, /* at least one digit met (condition of a valid scan) */
                dg; /* value of scanned decimal digit */
  long cn=0; /* result */
  unsigned long frac=0; /* store fractional part as integer */
  unsigned long fracScaler=0; /* store the maximum value fract will compete against */
  unsigned long binFrac; /* fractional binary shifter */
  
  short i, nbSpaces=0;

  if(pStr==NULL)
   return(0);
  /* skip trailing spaces */
  while(*pStr==' ' || *pStr=='\t')
   {
    pStr++;
    nbSpaces++;
   }
  /* scans each character */
  for(i=0; !bDone; i++)
   {
    switch (pStr[i])
     {
      /* leading '+': accepted otherwise stop */
      case '+':
       if(i>0)
        bDone=1;
       break;
      /* leading '-': accepted otherwise stop */
      case '-':
       if(i>0)
        bDone=1;
       else
        isPos=0; /* is not positive */
       break;
      /* '.': accepted once otherwise stop */
      case '.':
       if(fracScaler!=0)
        {
         bDone=1;
        }
       else
        {
         fracScaler=1; /* 10^n, n being the number of decimal fractions encountered */
        }
       break;
      case '\0':
       bDone=1;
       break;
      default:
       if(pStr[i]>='0' && pStr[i]<='9')
        {
         bAtLeastOneDigit=!0;
         if(pResult!=NULL) /* no need to do the math if nowhere to store result */
          {
           dg=pStr[i]-'0';
           if(!fracScaler) /* in integer part */
            {
             cn=qn_MULT10(cn); /* result makes room for one additional digit if possible */
             if(dg) /* no need to waste time adding 0! */
              cn-=(qn_Number)dg<<qn_fractionBits;
             if(cn>0) /* overflows! stops and report error */
              return(-2);
            }           
           else if(fracScaler<0xffffffff/10) /* in fractional part and still room left */
            {
             /* accumulates conversion in frac and fracScaler indicates the next multiple of ten */
             frac=qn_MULT10(frac); 
             frac+=(unsigned long)dg;
             fracScaler=qn_MULT10(fracScaler);
            }
          }
        }
       else
        bDone=0;
       break;
     }
   }
  /* now that we know how many characters are scanned, we only complete calculation
     and write in result if result address is okay and at least one valid digit scanned */
  if(pResult!=NULL && bAtLeastOneDigit)
   {
    if(fracScaler>1) /* fracScaler is 10^n the number of fractional digits encountered */
     {
      binFrac=1UL<<(qn_fractionBits-1); /* positions at the binary position */
      /* stores frac as long as there are some fractional bits used (qn_fractionBits>0)
         and as long as fractional position mask is not used up and some frac remains */
      while(qn_fractionBits && binFrac && frac)
       {
        frac <<= 1; /* tries to go beyond fracScaler */
        if(frac >= fracScaler) /* success: we crossed a decimal boundary */
         {
          cn -= binFrac;  /* accumulates in result */
          frac = frac - fracScaler; /* and substracts from remaining */
         }
        binFrac >>= 1; /* always balances frac multiplies by binFrac divides */
       }
      /* roundup either with remainings of frac (or with the full frac in case qn_fractionBits==0) */
      if(frac<<1 >= fracScaler)
       cn--;
     }
    if(isPos)
     {
      cn=-cn;
      if(cn<0) /* can happen because of different amplitudes between positive and negative numbers */
       return(-2);
     }
    *pResult=cn;
   }
  if(!bAtLeastOneDigit)
   return(-1);
  if(i>1)
   return(i+nbSpaces-1);
  return(0);
 }

/*****************************************************************************
    FUNCTION    qn_SPrintFDecimalNumber

    Description:
      Write pStr with decimal representation 'value'

    Entrance Conditions:
     qn_Init called
            
    Exit Conditions:
      
    args:
      value: number to convert
      pResult: address of buffer where to convert value
      nbDec: nb of decimals for conversion after rounding
             0 to qn_FORMAT_MAX_POS_DEC: number of decimals after '.'              
             -1 to -qn_FORMAT_MAX_NEG_DEC: printf integral value padding
                with (-nDec) zeroes. Example: value=3456.125, nDec=-2
                '3500' is written.
                Beware with small values: value=345.125, nDec=-2 gives "0"
                and value=745.125, nDec=-2 gives "1000"
             127: print all possible decimals, no rounding      
      nbWidth: width, left pads with spaces if conversion length is shorter than nbWidth
               0: ignored, no padding done
      bThousandSep: boolean, if true adds a comma for separating thousands
       
    returns:
      length of what is written in pResult
 *****************************************************************************/
short qn_SPrintFDecimalNumber(qn_Number value, char* pResult, signed char nbDec, unsigned char nbWidth, unsigned char bThousandSep)
{
  unsigned short i, j, low, digit, nb=0;
  unsigned long k, d, v;
 
  if(value<0)
   {
    pResult[nb++]='-';
    v=-value;
   }
  else
   v=value;
  if(nbDec>=0 && nbDec<=qn_FORMAT_MAX_POS_DEC) /* round on fractional decimals */
   {
    /* looks for 1/2 of position of 2^-dec on representation of fractional number */
    k=(formatDecimalPos[nbDec]+(1UL<<(POS_FRACT_RSHIFT-qn_fractionBits)))>>(POS_FRACT_RSHIFT-qn_fractionBits+1);
    v+=k; /* does the rounding */
   }
  else if(nbDec<0 && nbDec>=-qn_FORMAT_MAX_NEG_DEC && qn_fractionBits>=NEG_FRACT_LSHIFT)
   {
    /* no need to work on fractional part */
    /* we don't care of this part as the rounding is done solely on the integral part */
    v>>=qn_fractionBits;
    /* if we don't have v>=10^|nbDec|, this rounding brings zero */ 
    if(v+(unsigned long)(powersOfTen[-nbDec]>>1)>=(unsigned long)powersOfTen[-nbDec])
     {
      /* round v on 10^|nbDec|: ((v+10^|nbDec|/2)/10^|nbDec|)*10^|nbDec| */
      v=(v+(powersOfTen[-nbDec]>>1))/powersOfTen[-nbDec];
      v*=powersOfTen[-nbDec];
     }
    else
     v=0;
    /* restore decimal value without fractional part */
    v<<=qn_fractionBits;
   }
  /* this prevents "-0" to be displayed! */
  if(!v)
   nb=0; 
  /* first works on integral part */
  k= v>>qn_fractionBits;
  /* looks up low such as k<10^(low+1) */
  for(low=0; low<NB_OF_MULTIPLES_OF_TEN; low++)
   {
    if(k<(unsigned long)powersOfTen[low+1])
     break;
   }
  /* i indexes every digit from low-1 down to -1 */
  /* and operates a division of v by 10 through mutiplies and substracts */
  for(i=low-1; i!=0xfffe; i--)
   {
    if(i==0xffff) /* last digit */
     pResult[nb++]=(char)k+'0';
    else
     {
      d=powersOfTen[i+1];
      if(k<d) /* value to convert to small: digit is zero */
       pResult[nb++]='0';
      else 
       { /* look for d as multiple of 10^(i+1) such as value to convert is smaller */
        for(j=1; j<=10; j++, d+=(unsigned long)powersOfTen[i+1])
         {
          if(k<(d+(unsigned long)powersOfTen[i+1]))
           break;
         }
       /* d was the last multiple before we hit the last comparison can be removed from value */
       k-=d;
      /* j is the quotient */ 
       pResult[nb++]='0'+j;
      }
     /* 2^31 = 2,147,483,648: three thousand separator is enough */
     if(bThousandSep && (i==2 || i==5 || i==8))
      pResult[nb++]=',';
    }
   }
  /* calculates the fractional part if fractional digits required and defined */
  if(nbDec>0 && qn_fractionBits)
   {
    pResult[nb++]='.';
    k = v << (32-qn_fractionBits); /* align decimal point at left */
    low = (unsigned short)k;
    k   >>=  4;   /* shift to make room for a decimal digit */
    for (i=1; i<=10 && i<=nbDec; ++i)
     {
      digit = (k *= 10L) >> (32-4);
      low = (low & 0xf) * 10;
      k += ((unsigned long) (low>>4)) - ((unsigned long) digit << (32-4));
      pResult[nb++]=digit+'0';
     }
    }
 pResult[nb]='\0';
 /* adds space on left if nbWidth is great enough */
 if(nbWidth>nb)
  {
   j=nbWidth-nb;
   /* moves memory right of j positions */
   for(i=nbWidth; i>=j; i--)
    pResult[i]=pResult[i-j];
   /* and inserts j spaces */
   for(; i<0xffff; i--)
    pResult[i]=' ';
   nb=nbWidth;
  }
 return(nb);
}

static const unsigned long BitRankLUT[] = { 0xffff0000, 0xff00ff00, 0xf0f0f0f0, 0x30303030, 0xaaaaaaaa };
 
/*****************************************************************************
    FUNCTION    qn_RankOfHigherBit

    Description:
      Gets rank (0-31) of higher bit set to one in a long
    Note:
      Compared to a simpler implementation like this:
       {
        unsigned char r;
        for(r=31; r<=31; r--)
         if(value&(1UL<<r))
          break;
        return(r);
       }
      this function appears 35% faster and less dependant on rank 

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      value: long value under test
       
    returns:
      rank of 0xff if no bit set
 *****************************************************************************/
unsigned char qn_RankOfHigherBit(unsigned long value)
{
 unsigned char r=0;
 unsigned long v;
 if(value==0)
  return(0xff);
 if((v=value&0xffff0000)!=0)
  {
   value=v;
   r=16;
  }
 if((v=value&0xff00ff00)!=0)
  {
   value=v;
   r+=8;
  }
 if((v=value&0xf0f0f0f0)!=0)
  {
   value=v;
   r+=4;
  } 
 if((v=value&0xcccccccc)!=0)
  {
   value=v;
   r+=2;
  }
 if(value&0xaaaaaaaa)
  r+=1;
 return(r);
}

/*****************************************************************************
    FUNCTION    qn_RankOfLowerBit

    Description:
      Gets rank (0-31) of lower bit set to one in a long

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      value: long value under test
       
    returns:
      rank of 0xff if not bit set
 *****************************************************************************/
unsigned char qn_RankOfLowerBit(unsigned long value)
{
 unsigned char r=31;
 unsigned long v;
 if(value==0)
  return(0xff);
 if((v=value&0x0000ffff)!=0)
  {
   value=v;
   r-=16;
  }
 if((v=value&0x00ff00ff)!=0)
  {
   value=v;
   r-=8;
  }
 if((v=value&0x0f0f0f0f)!=0)
  {
   value=v;
   r-=4;
  } 
 if((v=value&0x33333333)!=0)
  {
   value=v;
   r-=2;
  }
 if(value&0x55555555)
  r-=1;
 return(r);
}

/*****************************************************************************
    FUNCTION    qn_Mul

    Description:
      Multiply two fractional numbers and return result

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      a, b numbers to multiply
      pErr if not NULL refers to a location where to store status of operation
          statuses: 0 (qn_RES_OK) -> okay,
                    1 (qn_RES_POVERFLOW) -> +overflow,
                    2 (qn_RES_NOVERFLOW) -> -overflow
       
    returns:
      Result of operation
 *****************************************************************************/
qn_Number qn_Mul(qn_Number a, qn_Number b, unsigned char* pErr)
 {
  unsigned char bNeg=0, qnf2=(qn_fractionBits);
  unsigned long m64, m32a, m32b;
  // compose signs and take absolute values
  if(a<0)
   {
    a=-a;
    bNeg=!0;
   }
  if(b<0)
   {
    b=-b;
    bNeg=!bNeg;
   }

  // 32-bitx32-bit = 64-bit multiply
  // ((a1<<16)+a0) * ((b1<<16)+b0)
  //  = (a1b1)<<32 + ((a0b1)<<16) + ((a1b0)<<16) + a0b0

  // first term: a1b1 in upper 32-bit register
  m64=((unsigned long)a>>16)*((unsigned long)b>>16);
  // second term: a0b1
  m32a=((unsigned long)(a&0xffff))*((unsigned long)b>>16);
  // split: most significant 16 bits in upper 32-bit register
  m64+=m32a>>16;
  // ... and least significant 16 bits in lower 32-bit register (unshifted)
  m32a&=0xffff;
  // third term: a1b0
  m32b=((unsigned long)a>>16)*((unsigned long)(b&0xffff));
  // split: most significant 16 bits in upper 32-bit register
  m64+=m32b>>16;
  // ... and least significant 16 bits in lower 32-bit register (unshifted)
  m32b&=0xffff;
  // accumulate unshifted middle terms
  m32a+=m32b;
  // add carry if any
  m64+=m32a>>16;
  // shift 16 bits of middle terms
  m32a<<=16;
  // last term: a0b0
  m32b=(unsigned long)(a&0xffff)*(unsigned long)(b&0xffff);
  // carry?
  if(m32a+m32b<m32a || m32a+m32b<m32b)
   m64++; // yes
  // accumulates in lower register
  // now result-64 in (m64:m32a)
  m32a+=m32b;
  // will shift 64-bit result by qn_fractionBits
  // first round-up at qn_fractionBits-1 bits
 #ifdef DEBUG
  #ifdef _MSC_VER
  { /* DEBUG */
   unsigned long long r=UInt32x32To64(a, b);
   if(m64!=(r>>32) || m32a!=(r&(unsigned long long)0xffffffff))
    printf("CRITICAL: ERROR in MULTIPLY\n");
  }
  #endif
 #endif
  if(qnf2>0)
   {
    if(m32a+(1UL<<(qnf2-1))<m32a)
     m64++;
    m32a+=1UL<<(qnf2-1);
   }
  // test first overflow conditions:
  // some bits set in upper 32-bit register after right shift
  if(m64&(0xffffffff<<qn_fractionBits))
   goto m_ovrflw;
  // qn_fractionBits right shift
  m32a=m64<<(32-qnf2) | m32a>>qnf2;
  // second condition of overflow above 31 bits:
  //  result positive and above 0x7fffffff
  //  result negative and above 0x80000000
  if(m32a&0x80000000)
   if(!bNeg || m32a>0x80000000)
    goto m_ovrflw;
  a = bNeg? -(qn_Number)m32a : (qn_Number)m32a;
  if(pErr!=NULL)
   *pErr=qn_RES_OK;
  return(a);
 m_ovrflw:
  if(pErr!=NULL)
   *pErr=bNeg? qn_RES_NOVERFLOW : qn_RES_POVERFLOW;
  return(bNeg? 0x80000000 : 0x7fffffff);
}

/*****************************************************************************
    FUNCTION    qn_Div

    Description:
      Divide two fractional numbers and return result

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      a, b numbers to divide
      pErr if not NULL refers to a location where to store status of operation
          statuses: 0 (qn_RES_OK) -> okay,
                    1 (qn_RES_POVERFLOW) -> +overflow,
                    2 (qn_RES_NOVERFLOW) -> -overflow,
                    3 (qn_RES_PINF) -> +infinity, 
                    4 (qn_RES_NINF) -> -infinity, 
                    5 (qn_RES_UNDEF) -> unknown (0/0) 
       
    returns:
      Result of operation
 *****************************************************************************/
qn_Number qn_Div(qn_Number a, qn_Number b, unsigned char* pErr)
 {
  unsigned char bNeg=0;
  unsigned long ua, ub, r;
  long acc=0;
  unsigned ha, hb;
  // compose signs and take absolute values
  if(a<0)
   {
    a=-a;
    bNeg=!0;
   }
  if(b<0)
   {
    b=-b;
    bNeg=!bNeg;
   }
  ua=a;
  ub=b;
  ha=qn_RankOfHigherBit(ua);
  hb=qn_RankOfHigherBit(ub);
  if(ha==255)
   if(hb==255)
    {
     if(pErr!=NULL)
      *pErr=qn_RES_UNDEF;
     return(0);
    }
   else
    {
     if(pErr!=NULL)
      *pErr=qn_RES_OK;
     return(0);
    }
  else
   if(hb==255)
    {
     if(pErr!=NULL)
      *pErr=bNeg? qn_RES_NINF : qn_RES_PINF;
     return(bNeg ? 0x80000000 : 0x7fffffff);
    }
  r=1<<qn_fractionBits; // r = 1.0000
  if(ua==ub)
   {
    acc=r;
    goto _end;
   }
  if(ua>ub)
   {
/*
   a = 0001 0110 1011 0100.1100 1000 0000 1000
   b = 0000 0000 1010 0101.1101 0011 0000 1010
   r = 0000 0000 0000 0001.0000 0000 0000 0000
   ha=28, hb=23 -> ha = 5
   r = 0000 0000 0010 0000.0000 0000 0000 0000  
   b = 0001 0100 1011 1010.0110 0001 0100 0000
 */
    ha-=hb; // ha will help level up ub to ua 
    ub<<=ha; // ub leveled up
    r<<=ha; // r acts as a multiplier
    if(r==0) // multiplier overflow (went beyond 2^31)
     {
      goto _overflows;
     }
   }
  else // ua < ub
   {
    ha=hb-ha; // ha will help level down ub to ua
    ub>>=ha; // ub leveled down
    r>>=ha; // r acts as a divider
   }
  while(ua && ub && r) // until there is something to substract from, to substract to, and multiplier is not too small
   {
    if(ua>=ub) // got a hit
     {
      ua-=ub; // remove what can be
      acc|=r; // accumulate multiplier 
     }
    // one rank lower
    r>>=1; 
    ub>>=1;
   }
 _end:
  // check some overflows which are not symmetrical between positive and negative
  if((acc==0x80000000 && !bNeg) || (unsigned long)acc>0x80000000)
   {
   _overflows:
    if(pErr!=NULL)
     *pErr=bNeg? qn_RES_NOVERFLOW : qn_RES_POVERFLOW;
    return(bNeg ? 0x80000000 : 0x7fffffff);
   }
  if(pErr!=NULL)
   *pErr=qn_RES_OK;
  return(bNeg?-acc:acc);
}

/*****************************************************************************
    FUNCTION    qn_Sign

    Description:
      Figures out sign of a fractional number

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      qn number under test
     
    returns:
      Result of operation:
        1: positive
        0: null
       -1: negative
       -2: minimum negative (meaning we have: qn equals -qn)
 *****************************************************************************/
signed char qn_Sign(qn_Number qn)
 {
  if(qn>0)
   return(1);
  else if(qn<0)
   {
    if(qn==0x80000000) // minimum negative
     return(-2);
    else
     return(-1);
   }
  return(0);
 }

/*****************************************************************************
    FUNCTION    qn_Cnv

    Description:
      Transforms one qn number defined with a certain number of fractional digits
      into a qn number of another number of fractional digits

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      a number to transform
      presentNbofFractionalDigits number of fractional digits a in defined with
      nextNbofFractionalDigits number of fractional digits result is defined with
      pErr if not NULL refers to a location where to store status of operation
          statuses: 0 (qn_RES_OK) -> okay,
                    1 (qn_RES_POVERFLOW) -> +overflow,
                    2 (qn_RES_NOVERFLOW) -> -overflow,
                    5 (qn_RES_UNDEF) -> unknown (wrong parameters) 
       
    returns:
      Result of operation
 *****************************************************************************/
qn_Number qn_Cnv(qn_Number qn, unsigned char presentNbofFractionalDigits,
                 unsigned char nextNbofFractionalDigits, unsigned char* pErr)
{
 unsigned char isNeg=0;
 unsigned char roundNeeded=0;
 unsigned char err=qn_RES_OK;
 unsigned char hrank;

 if(presentNbofFractionalDigits>qn_MAXFRACTBITS || nextNbofFractionalDigits>qn_MAXFRACTBITS)
  {
   if(pErr!=NULL)
    *pErr=qn_RES_UNDEF;
   return(qn);
  }
 if(qn) // qn null does not need processing 
  {
   if(presentNbofFractionalDigits<nextNbofFractionalDigits) // should shift left
    { // risk of overflow
     if((isNeg=(qn_Sign(qn)<0))!=0)
      qn=-qn;
     hrank=qn_RankOfHigherBit(qn);
     if(hrank+nextNbofFractionalDigits-presentNbofFractionalDigits==31 &&
       (qn<<(nextNbofFractionalDigits-presentNbofFractionalDigits))==0x80000000)
      qn=0x80000000; // minimum negative needs special processing
     else if(hrank+nextNbofFractionalDigits-presentNbofFractionalDigits>=31) // otherwise we should never reach bit 31
      {
       if(isNeg)
        {
         err=qn_RES_NOVERFLOW;
         qn=0x80000000;
        }
       else
        {
         err=qn_RES_POVERFLOW;
         qn=0x7fffffff;
        }
      }
     else
      qn<<=nextNbofFractionalDigits-presentNbofFractionalDigits;
     if(isNeg)
      qn=-qn;
    }
   else if(presentNbofFractionalDigits>nextNbofFractionalDigits) // should shift right
    {
     if(qn_Sign(qn)==-1) // minimum negative will be gently shifted, don't consider it specially 
      {
       isNeg=!0;
       qn=-qn;
      }
     else
      isNeg=0;
     if(presentNbofFractionalDigits-nextNbofFractionalDigits>=1) // at least one bit could be lost
      {
       if(qn & (1ul<<(presentNbofFractionalDigits-nextNbofFractionalDigits-1))) // see if higher lost bit is set
        roundNeeded=!0; // yes: rounding needed
      }
     // here minimum negative will be right shifted with negative sign extension, and as isNeg and roundNeeded are False
     // everything will be alright and we'll end up with a negative conversion  
     qn>>=presentNbofFractionalDigits-nextNbofFractionalDigits;
     if(roundNeeded)
      {
       qn++;
      // risk of rounding deemed null
      // because a was 0x7ffffff after turned positive, then at least shifted one it will turn into
      // at least 0x3ffffff, add one to it only makes 0x4000000: no overflow
      // 0x80000000 is not a problem either as it takes no rounding
   #if 0 // commented out - as explained above we don't fear any risk of overflow after right shift and +1 rounding
       if(qn&0x8000000) // overflow?
        { // yes (probably not possible anyway)
         if(isNeg)
          {
           err=qn_RES_NOVERFLOW;
           qn=0x80000000;
          }
         else
          {
           err=qn_RES_POVERFLOW;
           qn=0x7fffffff;
          }
        }
     #endif
      }
     if(isNeg) 
      qn=-qn;
    }
  }
 if(pErr!=NULL)
  *pErr=err;
 return(qn);
}  

/*****************************************************************************
    FUNCTION    qn_ToLong

    Description:
      Transforms one qn number into a long integer

    Entrance Conditions:
     None
    Exit Conditions:
     None

    args:
      qn number to transform
      op operation to perform - one of the following constants:
         TOLONG_OP_FLOOR  (1) floor(1.x) = 1, floor(-1.x)=-2 (x!=0)
         TOLONG_OP_CEIL   (2) ceil(1.x) = 2, ceil(-1.x)=-1 (x!=0)
         TOLONG_OP_ROUND  (3) round(1.9) = 2, round(-1.9)=-2, round(1.4) = 1, round(-1.4)=-1
         TOLONG_OP_TRUNC  (4) trunc(1.x) = 1, trunc(-1.x)=-1
         TOLONG_OP_EXCESS (5) excess(1.x) = 2, excess(-1.x)=-2 (x!=0)
     
    returns:
      Result of operation
 *****************************************************************************/
long qn_ToLong(qn_Number qn, unsigned char op)
{
 signed char sign; // sign of a: -2 minimum negative, -1 negative, 0 null, 1 positive
 qn_Number qPart;

 if(qn_fractionBits==0) // slight performance loss, but simplifies implementation: no fear for overflows
  return(qn);
 if(op==TOLONG_OP_ROUND) // qn_Cnv takes care of roundings already
  return(qn_Cnv(qn, qn_fractionBits, 0, NULL));
 qPart=qn & ((1<<qn_fractionBits)-1UL);
 switch(op)
  {
   case TOLONG_OP_FLOOR:
    return(qn>>qn_fractionBits); // crush fractional part, negative is extended by right shift   
   case TOLONG_OP_CEIL:
    if(qPart>0)
     return((qn>>qn_fractionBits)+1); // any fractional part, go one unit up
    return(qn>>qn_fractionBits);
   default:
    break;
  }
 // have to work on the absolute value for truncation and excess
 if((sign=qn_Sign(qn))==-1)
  qn=-qn;
 switch(op)
  {
   case TOLONG_OP_TRUNC:
    qn>>=qn_fractionBits; // go to next integer closer to zero
    if(sign==-1) // in case of sign being -2, qn is shifted with negative extension
     qn=-qn;
    return(qn);
   case TOLONG_OP_EXCESS:
    qn=qn>>qn_fractionBits;
    if(qPart)
     qn++; // go to next integer further to zero
    if(sign==-1) // in case of sign being -2, qPart==0 and qn is shifted with negative extension
     qn=-qn;
    return(qn);
   default:
    break;
  }
 return(qn>>qn_fractionBits);
}
/******************************************************************************
  End of file
*******************************************************************************/

