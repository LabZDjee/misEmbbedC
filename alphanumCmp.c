#include "alphanumCmp.h"

#include <stdlib.h>

static const ANCnsSProfile defProfile = { 0, -1, NULL };

int ANCnsIsDigit(char c)
{
 return c >= '0' && c <= '9';
}

unsigned long ANCnsGetValue(const char* str, int maxLength, ANCnsStrSize* pOffset)
{
 unsigned long result = 0;
 ANCnsStrSize idx;
 char c;

 if (pOffset == NULL)
  {
   idx = 0;
  }
 else
  {
   idx = *pOffset;
  }
 while ((maxLength < 0 || idx < maxLength) && (c = str[idx]) != '\0')
  {
   if (!ANCnsIsDigit(c))
    {
     break;
    }
   result = 10 * result + c - '0';
   idx++;
  }
 if (pOffset != NULL)
  {
   if (idx > *pOffset)
    {
     *pOffset = idx;
    }
   else
    {
     *pOffset = 0;
    }
  }
 return result;
}

char ANCnsToUpper(char c)
{
 if (c >= 'a' && c <= 'z')
  {
   c += 'A' - 'a';
  }
 return c;
}

int ANCnsStrChr(char c, const char* list)
{
 int i;
 char cInList;
 if (list != NULL)
  {
   for (i = 0; (cInList = list[i]) != '\0'; i++)
    {
     if (c == cInList)
      {
       return i;
      }
    }
  }
 return -1;
}

int ANCnsExtract(const char* str, const ANCnsSProfile* profile, unsigned long* pResult, ANCnsStrSize* pOffset)
{
 unsigned long gv;
 char c;
 ANCnsStrSize offset = pOffset == NULL ? 0 : *pOffset;
 /* end of string to extract not always guaranteed below, so check now */
 if (str == NULL || (c = str[offset]) == '\0' || (profile->maxLength >= 0 && offset >= profile->maxLength))
  {
   return 0;
  }
 if (profile == NULL)
  {
   profile = &defProfile;
  }
 gv = ANCnsGetValue(str, profile->maxLength, &offset);
 if (offset > 0)
  {
   if (pOffset != NULL)
    {
     *pOffset = offset;
    }
   if (pResult != NULL)
    {
     *pResult = gv;
    }
   return 2;
  }
 offset = pOffset == NULL ? 0 : *pOffset;
 if (profile->pSpaceDef != NULL)
  {
   for (gv = offset;
        (profile->maxLength < 0 || offset < profile->maxLength) && ANCnsStrChr(c, profile->pSpaceDef) >= 0;
        )
    {
     c = str[++offset];
    }
   if (offset > gv)
    {
     if (pResult != NULL)
      {
       *pResult = offset - gv;
      }
     if (pOffset != NULL)
      {
       *pOffset = offset;
      }
     return 1;
    }
  }
 if (profile->bCaseInsensitive)
  {
   c = ANCnsToUpper(c);
  }
 if (pResult != NULL)
  {
   *pResult = (unsigned char)c;
  }
 if (pOffset != NULL)
  {
   do
    offset++;
   while ((profile->maxLength < 0 || offset < profile->maxLength) &&
          (c = str[offset]) != '\0' &&
          !ANCnsIsDigit(c) &&
          (profile->pSpaceDef == NULL || ANCnsStrChr(c, profile->pSpaceDef) < 0));
   *pOffset = offset;
  }
 return 3;
}

int ANCnsStrCmp(const char* s1, const char* s2, const ANCnsSProfile* profile)
{
 ANCnsStrSize of1 = 0, of2 = 0, of1_0, of2_0, idx1, idx2;
 unsigned long v1, v2;
 int r1, r2;
 char c1, c2;

 if (profile == NULL)
  {
   profile = &defProfile;
  }
 do
  {
   of1_0 = of1;
   of2_0 = of2;
   r1 = ANCnsExtract(s1, profile, &v1, &of1);
   r2 = ANCnsExtract(s2, profile, &v2, &of2);
   if (r1 == r2)
    {
     switch (r1)
      {
       case 0: /* both empty at offset */
        return 0;
       case 3: /* characters (non digit, non space extracted at offset */
        /* regular 'strcmp' up to the minimum of characters found */
        for (idx1 = of1_0, idx2 = of2_0; idx1 < of1 && idx2 < of2; idx1++, idx2++)
         {
          c1 = s1[idx1];
          c2 = s2[idx2];
          if (profile->bCaseInsensitive)
           {
            c1 = ANCnsToUpper(c1);
            c2 = ANCnsToUpper(c2);
           }
          if (c1 != c2)
           {
            return c1 - c2; /* difference of characters, positive meaning s1 > s2 */
           }
         }
        if (idx1 < of1)
         {
          return s1[idx1]; /* more on s1, return positive */
         }
        if (idx2 < of2)
         {
          return -s2[idx2]; /* more on s1, return negative */
         }
        break;
       case 2: /* digit extracted at offset */
        if (v1 != v2) /* if not equal, stop here */
         {
          if (!(int)(v1 - v2)) /* as range of ANCnsStrSize and int can vary... */
           {
            return v1 > v2 ? 1 : -1; /* ... could be result can be 0. avoiding this case */
           }
          return v1 - v2; /* again positive if s1 > s2 */
         }
        break;
       case 1:
        break;
      }
    }
   else
    {
     return r1 - r2; /* hierarchy of what extract returns is such this difference reflects s1>s2 right */
    }
  }
 while (1);
}

/* return true if c is a decimal digit */
#define ISNUM(c) ((unsigned)c - '0' < 10)

int ANCnsFastStrncmp(const char* s1, const char* s2, ANCnsStrSize maxLength)
{
 /* this function scans strings to separate them into groups of non-digits or digits */
 int i1 = 0, /* overall index on s1 */
     isNum1, /* if current group on s1 is numeric */
     i11; /* index when looking for a group in s1 */
 int i2 = 0, /* overall index on s2 */
     isNum2, /* if current group on s2 is numeric */
     i21; /* index when looking for a group in s2 */
 int iz; /* index on the first non-zero decimal in a group of decimals (or only zero found) */
 int dif;
 char c;
 int notEnd = 0; /* scan for groups not finished */

 do
  {
   /* scans a group on s1 from i1 onwards */
   for (isNum1 = 0, i11 = i1; (notEnd = (i11 < maxLength && (c = s1[i11]) != '\0')) != 0; i11++)
    {
     if (ISNUM(c)) /* found decimal digit */
      {
       if (i11 == i1) /* first digit found in group? */
        {
         isNum1 = 1; /* enter a group of decimals */
        }
       else
        {
         break; /* leave a group of a non decimals because on decimal met here */
        }
       iz = i11; /* iz will be increased will leading zeroes */
       /* will capture group of decimals here */
       do
        {
         if (c == '0' && iz == i11)
          {
           iz++; /* updates index to non-zero because zero found */
          }
         i11++;
        }
       while ((notEnd = (i11 < maxLength && (c = s1[i11]) != '\0')) != 0 && ISNUM(c));
       /* capture finished: went to far? */
       if (iz == i11)
        {
         iz--; /* yes: points to the only zero */
        }
       i1 = iz; /* this ensures leading zeroes are skipped */
       break;
      }
    }
   /* scans a group on s2 from i2 onwards */
   for (isNum2 = 0, i21 = i2; (notEnd = (i21 < maxLength && (c = s2[i21]) != '\0')) != 0; i21++)
    {
     if (ISNUM(c)) /* found decimal digit */
      {
       if (i21 == i2) /* first digit found in group? */
        {
         isNum2 = 1; /* enter a group of decimals */
        }
       else
        {
         break; /* leave a group of a non decimals because on decimal met here */
        }
       iz = i21; /* iz will be increased will leading zeroes */
       /* will capture group of decimals here */
       do
        {
         if (c == '0' && iz == i21)
          {
           iz++; /* updates index to non-zero because zero found */
          }
         i21++;
        }
       while ((notEnd = (i21 < maxLength && (c = s2[i21]) != '\0')) != 0 && ISNUM(c));
       /* capture finished: went to far? */
       if (iz == i21)
        {
         iz--; /* yes: points to the only zero */
        }
       i2 = iz; /* this ensures leading zeroes are skipped */
       break;
      }
    }
   if (isNum1)
    {
     if (!isNum2)
      {
       /* isNum2==0, can be because no character found in s2 or a non digits */
       return i2 == i21 ? 1 : -1; /* non-digits have priority over digits */
      }
     dif = (i11 - i1) - (i21 - i2); /* because leading zero skipped, shortest digit string means less */
     if (dif) /* so, no need to go any further */
      {
       return dif;
      }
    }
   else if (isNum2)
    {
     /* isNum1==0, can be because no character found in s1 or a non digits */
     return i1 == i11 ? -1 : 1; /* non-digits have priority over digits */
    }
   while (1)
    {
     if (i1 == i11) /* exhausted s1? */
      {
       if (i2 != i21)
        {
         return -1; /* not both exhausted */
        }
       break;
      }
     if (i2 == i21) /* exhausted s2? */
      {
       return 1;
      }
     dif = s1[i1] - s2[i2]; /* classical comparison */
     if (dif)
      {
       return dif;
      }
     /* no difference, proceed with next */
     i1++;
     i2++;
    }
  }
 while (notEnd);
 return 0;
}

#ifdef MODULE_TESTS

#include <stdio.h>

/* defines normalized result for comparions of two strings: s1<s2, s1==s2, s1>s2 */
enum { ANCnsLess = -1, ANCnsEqual = 0, ANCnsGreater = 1 };

/* return -1, 0, 1: according to sign of val (strictly negative, null, or strictly positive) */
static int _ANCnsSign(int val) {
 return val < 0 ? ANCnsLess : (val > 0 ? ANCnsGreater : ANCnsEqual);
}

/* define a test items for main comparison function */
typedef struct _ANCnsTestVector
{
 const char* s1;    /* first string to compare */
 int expected;    /* should be ANCnsLess, ANCnsEqual=0, or ANCnsGreater */
 const char* s2;    /* second string to compare */
 ANCnsSProfile p;    /* profile (parameters) for compare */
}ANCnsTestVector;

/* set of test items for main compare */
static ANCnsTestVector _ANCns_testVectors[] =
{
 { "abc", ANCnsLess, "abg", { 0, -1, " \t_" } },
 { "abc", ANCnsLess, "abc1", { 0, -1, " \t_" } },
 { "ab110", ANCnsGreater, "ab19", { 0, -1, " \t_" } },
 { "ab 110", ANCnsLess, "ab19", { 0, -1, " \t_" } },
 { "ab__110", ANCnsGreater, "ab\t19", { 0, -1, " \t_" } },
 { "abc ", ANCnsLess, "abc1", { 0, -1, " \t_" } },
 { "ab cd 008", ANCnsEqual, "ab__cd__8", { 0, -1, " \t_" } },
 { "ab cd 008", ANCnsLess, "ab__cd__8b", { 0, -1, " \t_" } },
 { " ab", ANCnsLess, " abc", { 0, -1, " \t_" } },
 { "ab", ANCnsGreater, "18", { 0, -1, " \t_" } },
 { " abc", ANCnsEqual, "  abc", { 0, -1, " \t_" } },
 { "abc", ANCnsGreater, "  abc", { 0, -1, " \t_" } },
 { "ab cd zzz", ANCnsEqual, "AB_cd_aaa", { 1, 6, " \t_" } },
 { "ab cd", ANCnsLess, "AB_cd", { 1, 6, NULL } },
 { "12", ANCnsLess, "aaa", { 1, -1, " " } },
 { "12\t\t", ANCnsGreater, "\t\t\t12", { 1, -1, "\t" } },
 { "12__", ANCnsGreater, "\\aaa", { 1, -1, "-_\\" } },
 { "abc ", ANCnsLess, "abc  ", { 0, -1, "" } },
 { " abcd", ANCnsGreater, " abc", { 0, -1, " " } },
 { NULL, 0, NULL, { 0, -1, NULL } } /* last should be defined like this */
};

/* define a test item for fast comparison function */
typedef struct _ANCnsTestVectorForFastCmp
{
 const char* s1;    /* first string to compare */
 int expected;    /* should be ANCnsLess, ANCnsEqual=0, or ANCnsGreater */
 const char* s2;    /* second string to compare */
 ANCnsStrSize maxLength;
}ANCnsTestVectorForFastCmp;

/* set of test items for fast compare */
static ANCnsTestVectorForFastCmp _ANCns_testVectorsForFastCmp[] =
{
 { "hello001023ab12c0", ANCnsEqual, "hello01023ab012c000", 20 },
 { "hello001023ab12c0", ANCnsLess, "hello01024ab12c0", 20 },
 { "hello001023az12c0", ANCnsGreater, "hello1023aa12c0", 20 },
 { "hello1023az12c0", ANCnsLess, "helloaa12c0", 20 },
 { "hello", ANCnsLess, "helloaa", 20 },
 { "hello1023ac12c0", ANCnsEqual, "hello1023ab12c0", 10 },
 { "", ANCnsLess, "h", 10 },
 { "", ANCnsEqual, "", 10 },
 { "12", ANCnsGreater, "", 10 },
 { "a", ANCnsGreater, "1", 10 },
 { "boo", ANCnsLess, "foo", 1 },
 { "bar", ANCnsEqual, "foo", 0 },
 { NULL, 0, NULL, 0 } /* last should be defined like this */
};

static void _ANCnsPrintStrWithEscapes(const char* p);

int main(int argc, char* argv[])
{
 int i, t1, t2;
 int fnb, f, ffnb;
 int r;
 int verbose = 1;
 /* unbuffered standard output (for Eclipse under Windows) */
 setvbuf(stdout, NULL, _IONBF, 0);
 printf("tests of function \"ANCnsStrCmp\"\n");
 for (i = fnb = 0; _ANCns_testVectors[i].s1 != NULL; i++)
  {
   if (verbose)
    {
     printf("t%i: comparing ", i + 1);
     _ANCnsPrintStrWithEscapes(_ANCns_testVectors[i].s1);
     printf(" with ");
     _ANCnsPrintStrWithEscapes(_ANCns_testVectors[i].s2);
     printf("\n case insensitive: %s, max length: %ld, space definition: ",
            _ANCns_testVectors[i].p.bCaseInsensitive ? "yes" : "no",
            (long)_ANCns_testVectors[i].p.maxLength);
     _ANCnsPrintStrWithEscapes(_ANCns_testVectors[i].p.pSpaceDef);
     printf("\n");
    }
   r = ANCnsStrCmp(_ANCns_testVectors[i].s1, _ANCns_testVectors[i].s2, &_ANCns_testVectors[i].p);
   f = 0;
   if (_ANCnsSign(r) != _ANCnsSign(_ANCns_testVectors[i].expected))
    {
     f = !0;
     fnb++;
    }
   if (verbose)
    {
     printf(" direct comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsStrCmp(_ANCns_testVectors[i].s2, _ANCns_testVectors[i].s1, &_ANCns_testVectors[i].p);
   f = 0;
   if (_ANCnsSign(-r) != _ANCnsSign(_ANCns_testVectors[i].expected))
    {
     f = !0;
     fnb++;
    }
   if (verbose)
    {
     printf(" reversed comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsStrCmp(_ANCns_testVectors[i].s1, _ANCns_testVectors[i].s1, &_ANCns_testVectors[i].p);
   f = 0;
   if (r)
    {
     f = !0;
     fnb++;
    }
   if (verbose)
    {
     printf(" first self comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsStrCmp(_ANCns_testVectors[i].s2, _ANCns_testVectors[i].s2, &_ANCns_testVectors[i].p);
   f = 0;
   if (r)
    {
     f = !0;
     fnb++;
    }
   if (verbose)
    {
     printf(" second self comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
  }
 printf("Nb of tests: %i, Nb of failures: %i\n", t1 = 4 * i, fnb);
 printf("tests of function \"ANCnsFastStrncmp\"\n");
 for (i = ffnb = 0; _ANCns_testVectorsForFastCmp[i].s1 != NULL; i++)
  {
   if (verbose)
    {
     printf("t%i: comparing ", i + 1);
     _ANCnsPrintStrWithEscapes(_ANCns_testVectorsForFastCmp[i].s1);
     printf(" with ");
     _ANCnsPrintStrWithEscapes(_ANCns_testVectorsForFastCmp[i].s2);
     printf("\n max length: %ld\n",
            (long)_ANCns_testVectorsForFastCmp[i].maxLength);
    }
   r = ANCnsFastStrncmp(_ANCns_testVectorsForFastCmp[i].s1, _ANCns_testVectorsForFastCmp[i].s2, _ANCns_testVectorsForFastCmp[i].maxLength);
   f = 0;
   if (_ANCnsSign(r) != _ANCnsSign(_ANCns_testVectorsForFastCmp[i].expected))
    {
     f = !0;
     ffnb++;
    }
   if (verbose)
    {
     printf(" direct comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsFastStrncmp(_ANCns_testVectorsForFastCmp[i].s2, _ANCns_testVectorsForFastCmp[i].s1, _ANCns_testVectorsForFastCmp[i].maxLength);
   f = 0;
   if (_ANCnsSign(-r) != _ANCnsSign(_ANCns_testVectorsForFastCmp[i].expected))
    {
     f = !0;
     ffnb++;
    }
   if (verbose)
    {
     printf(" reversed comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsFastStrncmp(_ANCns_testVectorsForFastCmp[i].s1, _ANCns_testVectorsForFastCmp[i].s1, _ANCns_testVectorsForFastCmp[i].maxLength);
   f = 0;
   if (r)
    {
     f = !0;
     ffnb++;
    }
   if (verbose)
    {
     printf(" first self comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
   r = ANCnsFastStrncmp(_ANCns_testVectorsForFastCmp[i].s2, _ANCns_testVectorsForFastCmp[i].s2, _ANCns_testVectorsForFastCmp[i].maxLength);
   f = 0;
   if (r)
    {
     f = !0;
     ffnb++;
    }
   if (verbose)
    {
     printf(" second self comparison returned %d - %s\n", r, f ? "FAILURE" : "pass");
    }
  }
 printf("Nb of tests: %i, Nb of failures: %i\n", t2 = 4 * i, ffnb);
 printf(">*> Total number of tests: %i, total number of failures: %d\n", t1 + t2, fnb + ffnb);
 return -(fnb + ffnb);
}

static void _ANCnsPrintStrWithEscapes(const char* p)
{
 int i;
 char c;
 char* pF;
 if (p == NULL)
  {
   printf("<NULL>");
   return;
  }
 printf("\"");
 for (i = 0; (c = p[i]) != '\0'; i++)
  {
   pF = "\\x02x";
   if (c > 31 && c < 128)
    {
     printf("%c", c);
     continue;
    }
   else
    {
     switch (c)
      {
       case '\a': pF = "\\a"; break;
       case '\b': pF = "\\b"; break;
       case '\f': pF = "\\f"; break;
       case '\n': pF = "\\n"; break;
       case '\r': pF = "\\r"; break;
       case '\t': pF = "\\t"; break;
       case '\v': pF = "\\v"; break;
       case '\\': pF = "\\"; break;
       default: break;
      }
    }
   printf(pF, c);
  }
 printf("\"");
}

#endif /* MODULE_TESTS */
