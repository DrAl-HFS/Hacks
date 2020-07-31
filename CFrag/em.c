// em.c - Simple 1D Expectation Maximisation for distribution modelling.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July 2020

#include <math.h>
#include <stdlib.h>

/**/

typedef double WF; // Wide float type
typedef struct s_GM { WF p,m,sd; } GM; // Gaussian (mixture) Model descriptor
typedef struct s_GK { WF k[3]; } GK;   // GMM coefficients for efficient evaluation
typedef struct s_M2 { WF m[3]; } M2;   // Moment sums to order 2
typedef struct s_MB { void *p; size_t bytes; } MB;
typedef struct
{
   MB mb;   // Buffer
   GM *pR;  // Result (intermediate)
   GK *pGK; // Working coefficient
   const WF *pO; // Observations (input)
   WF *pE;     // expectation data
   M2 *pM2;    // Moment accumulation
   int maxM, maxO; //
} WorkCtx;


/**/

// Float vector functions (mostly scalar reductions)
//WF sumNF (const WF x[], const int n) { WF t= (n>0)?x[0]:0; for (int i=1; i<n; i++) { t+= x[i]; } return(t); }
//WF dotNF (const WF x[], const WF y[], const int n) { WF t= (n>0)?x[0]*y[0]:0; for (int i=1; i<n; i++) { t+= x[i] * y[i]; } return(t); }

WF sumStrideNF (const WF w[], const int s, const int n) { WF t= (n>0)?w[0]:0; for (int i=1; i<n; i++) { t+= w[i*s]; } return(t); }
WF sumIProdStrideNF (const WF w[], const int s, const int n) { WF t= 0; for (int i=1; i<n; i++) { t+= i * w[i*s]; } return(t); }
WF sumISSDStrideNF (const WF w[], const int s, const int n, const WF x0)
{  WF t= 0;
   for (int i=0; i<n; i++)
   {
      const WF di= i - x0;
      t+= di * di * w[i*s];
   }
   return(t);
} // sumISSDStrideNF

// Accumulate parallel sets of individually weighted moments of constant x
void accumM2NF (M2 m[], const WF w[], const WF x, const int n)
{
   WF x2= x * x;
   for (int i=0; i<n; i++)
   {
      m[i].m[0]+= w[i]; // x^0=1
      m[i].m[1]+= x * w[i];
      m[i].m[2]+= x2 * w[i];
   }
} // accumM2NF

void scaleNF (WF r[], const WF x[], const int n, const WF k) { for (int i=0; i<n; i++) { r[i]= x[i] * k; } }

// Gaussian model functions
const WF K0= 1.0 / sqrt(2 * M_PI);
// Convert model descriptor to coefficients for efficient evaluation
void getGK (WF k[3], const GM *pGM)
{  // assume ((pGM->p > EPS) && (pGM->sd > EPS))
   k[0]= pGM->p * K0 / pGM->sd;
   k[1]= pGM->m;
   k[2]= -1 / (2 * pGM->sd * pGM->sd);
} // getGK

// Convert moments to model descriptors
// NB: single pass variance estimation is not robust where sample values vary
// by orders of magnitude. (Not the case in this application.)
int setNGM (GM *pGM, const M2 m[], const int n) // , const int dof ??
{
   int nValid= 0;
   for (int i= 0; i<n; i++)
   {
      pGM[i].p= m[i].m[0];
      if (pGM[i].p > 0)
      {
         const WF rp=   1.0 / pGM[i].p;
         const WF mean= m[i].m[1] * rp;
         const WF ssd=  m[i].m[2] - m[i].m[1] * mean;
         if (ssd > 0)
         {
            pGM[i].m= mean;
            pGM[i].sd= sqrt(ssd * rp);
            nValid++;
         }
      }
   }
   return(nValid);
} // setNGM

// Evaluate n models for given x, storing individual results and computing sum
WF evalNGK (WF p[], const WF x, const GK gk[], const int n) // gk=[kP,M,kV]
{
   WF t= 0;
   for (int i= 0; i<n; i++)
   {  // assume gk[i][0] > EPS
      const WF xm= x - gk[i].k[1];
      t+= p[i]= gk[i].k[0] * exp(gk[i].k[2] * xm * xm);
   }
   return(t);
} // evalGK


/**/

// All-in-one EM pass with minimal memory usage
int em (const WorkCtx *pC, const GK gk[], const int nGK, const WF pmf[], const int nPMF)
{
   for (int i= 0; i<nPMF; i++)
   {
      WF s= evalNGK(pC->pE, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(pC->pE, pC->pE, nGK, pmf[i] / s);
         // Accumulate as moments of order 0,1,2
         accumM2NF(pC->pM2, pC->pE, i, nGK);
      }
   }
   // Convert moments to Gaussian model descriptors
   return setNGM(pC->pR, pC->pM2, nGK);
} // em

// Separate E,M passes requiring large buffer for intermediate results
void expect (WF p[], const GK gk[], const int nGK, const WF pmf[], const int nPMF)
{
   for (int i= 0; i<nPMF; i++)
   {
      const int j= nGK*i;
      WF s= evalNGK(p+j, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(p+j, p+j, nGK, pmf[i] / s);
      }
   }
} // expect

int maximise (GM *pR, const WF p[], const int nGK, const int nPMF) // m0,m1,m2
{
   int nK= 0;
   for (int i= 0; i<nGK; i++)
   {
      pR[i].p= sumStrideNF(p+i, nGK, nPMF);
      if (pR[i].p > 0)
      {  // Classic 2-pass measurement of mean&variance
         const WF rp= 1.0 / pR[i].p;
         pR[i].m= sumIProdStrideNF(p+i, nGK, nPMF) * rp;
         pR[i].sd= sqrt(sumISSDStrideNF(p+i, nGK, nPMF, pR[i].m) * rp);
         nK++;
      }
   }
   // if (nK < nGK) remove degenerates
   return(nK);
} // maximise

// int to float vector routines
WF sumNIF (const int x[], const int n) { WF s= x[0]; for (int i=1; i<n; i++) { s+= x[i]; } return(s); }

void scaleNIF (WF r[], const int x[], const int n, const WF s) { for (int i=0; i<n; i++) { r[i]= s * x[i]; } }

int normP1NIF (WF r[], const int x[], const int n)
{
   WF s= sumNIF(x,n);
   if (0 != s) { scaleNIF(r,x,n,1.0/s); return(n); }
   return(0);
} // normP1NIF

size_t alignPO2 (const size_t s, const size_t b)
{
   size_t a= (1<<b)-1;
   return((s+a) & ~a);
} // alignPO2

const WorkCtx * initWC (WorkCtx *pWC, const WF *pO, const size_t maxO, const size_t maxM)
{
   int nE= maxO * maxM;
   pWC->mb.bytes= nE * sizeof(WF) + maxM * (sizeof(GM) + sizeof(GK) + sizeof(M2));
   if (NULL == pO) { pWC->mb.bytes+= sizeof(WF) * maxO; }
   pWC->mb.bytes= alignPO2(pWC->mb.bytes, 12); // round up 4K
   pWC->mb.p= malloc(pWC->mb.bytes);
   if (pWC->mb.p)
   {
      if (NULL == pO)
      {
         pWC->pO= pWC->mb.p;
         pWC->pR= (void*)(pWC->pO + maxO);
      }
      else
      {
         pWC->pO= pO;
         pWC->pR= pWC->mb.p;
      }
      pWC->pGK= (void*)(pWC->pR + maxM);
      pWC->pE=  (void*)(pWC->pGK + maxM);
      pWC->pM2= (void*)(pWC->pE + nE);
      pWC->maxM= maxM;
      pWC->maxO= maxO;
      return(pWC);
   }
   return(NULL);
} // initWC

void freeWC (WorkCtx *pWC)
{
   if (pWC->mb.p)
   {
      free(pWC->mb.p);
      pWC->mb.p= NULL;
      //memset(pWC, 0, sizeof(*pWC));
   }
} // freeWC

#ifndef LIB_TARGET

#include "emTest.c"

int main (int argc, char *argv[])
{
   int r= 0;
   WorkCtx wc;
   const WorkCtx *pWC= initWC(&wc,NULL,32,2);

   if (pWC)
   {
      genObs((WF*)(pWC->pO), pWC->maxO, pWC->maxM);
      r= t2(pWC);
      freeWC(&wc);
   }
   return(r);
} // main

#endif
