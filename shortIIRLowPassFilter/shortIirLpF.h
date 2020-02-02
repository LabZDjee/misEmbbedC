#ifndef __SHORTIIRLPF_H__
#define __SHORTIIRLPF_H__

/*

   IIR (Infinite Impulse Response) First Order Low Pass Filter on short int [-32768; 32767]

   Implies a 'divider' which is a positive non-null integer which defines the 1/n proportion of new input:
     output(n) = input(n) / n + output(n-1) x (n-1) / n

 */

/* struct which internally stores management data: don't mess with them normally */
typedef struct _tShortIIRLowPass {
 long acc;
 unsigned short divider;
 unsigned short halfDivider;
} tShortIIRLowPass;

/*
 should be called to (re-)initialize filter at address pFilter
  divider: defines 1/divider proportion taken by new inputs
  initial: value pre-loaded to the filter
*/
void shortIIRLowPassInit(unsigned char divider, short initial, tShortIIRLowPass* pFilter);

/*
 add a newValue to the filter
*/
void shortIIRLowPassInput(short newValue, tShortIIRLowPass* pFilter);

/*
 get current filter output value
*/
short shortIIRLowPassGet(tShortIIRLowPass* pFilter);

#endif
