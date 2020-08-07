// em.h - Definitions for simple 1D Expectation Maximisation.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July-August 2020

#ifndef EM_H
#define EM_H

#include <math.h>
#include <string.h> // memcpy
#include <stdlib.h> // malloc
#include <stdio.h>

/***/

#define GM_NK 3
#define GM_NM 3

//typedef int bool;
typedef unsigned char U8;
typedef double WF; // Wide float type
typedef struct s_GM { WF p,m,sd; } GM; // Gaussian (mixture) Model descriptor
typedef struct s_GK { WF k[GM_NK]; } GK;   // GMM coefficients for efficient evaluation
typedef struct s_M2 { WF m[GM_NM]; } M2;   // Moment sums to order 2
typedef struct s_MB { union { void *p; size_t w; }; size_t bytes; } MB;
//typedef union u_EMCPD { int w; struct { U8 f; U8 n; U8 pad[2]; }; } EMCPD; // Client Procedure Descriptor???

typedef struct
{
   MB mb, ws;   // Total buffer & extra misc workspace (for testing)
   GM *pR;  // Result (intermediate)
   GK *pGK; // Working coefficient
   const WF *pO; // Observations (input)
   WF *pE;     // expectation data
   M2 *pM2;    // Moment accumulation
   size_t maxM, maxO, maxE; //
} WorkCtx;


/***/

// old style hacky macros
#define MIN(a,b) ((a)<(b)?(a):(b))
#define SWAP(_t,a,b) { _t t=a; a= b; b= t; }
#define XSWAP(a,b) { a^= b; b^= a; a^= b; }

// int -> float vector functions
extern WF sumNIF (const int x[], const int n);
extern void scaleNIF (WF r[], const int x[], const int n, const WF s);
extern int normP1NIF (WF r[], const int x[], const int n);

// typical float vector functions
extern void diffNF (WF d[], const WF x[], const WF y[], const int n);
extern void scaleNF (WF s[], const WF x[], const int n, const WF k);
extern void prodNF (WF r[], const WF x[], const WF y[], const int n);

// float vector -> scalar reductions
extern WF sumNF (const WF x[], const int n);
// Summation by sign [-ve,+ve] returns absolute sum
extern WF addSplitSumNF (WF ss[2], const WF x[], const int n);


extern void getNGK (GK gk[], const GM gm[], const int n);
extern int estGM (GM gm[], int *pI, const int maxM, const WF f[], const int nF, const WF w);

extern int em (GM rgm[], const GK gk[], const int nGK, const WorkCtx *pC);
// Separate E,M passes requiring large [nM*nO] buffer for intermediate results
extern void expect (WF e[], const GK gk[], const int nGK, const WF pmf[], const int nPMF);
extern int maximise (GM rgm[], const WF e[], const int nGK, const int nPMF);

extern int validMB (const MB *pMB, const size_t bytes);

#endif // EM_H
