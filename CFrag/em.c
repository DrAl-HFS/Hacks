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
typedef struct s_MB { union { void *p; size_t w; }; size_t bytes; } MB;
typedef struct
{
   MB mb, ws;   // Total buffer & extra misc workspace
   GM *pR;  // Result (intermediate)
   GK *pGK; // Working coefficient
   const WF *pO; // Observations (input)
   WF *pE;     // expectation data
   M2 *pM2;    // Moment accumulation
   size_t maxM, maxO, maxE; //
} WorkCtx;

#define MIN(a,b) ((a)<(b)?(a):(b))

/**/

// Float vector functions (mostly scalar reductions)
WF sumNF (const WF x[], const int n) { WF t= (n>0)?x[0]:0; for (int i=1; i<n; i++) { t+= x[i]; } return(t); }
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

void setNM2 (M2 m[], const int n, const WF x)
{  //memset(pWC, 0, sizeof(M2));
   for (int i=0; i<n; i++)
   {
      m[i].m[0]= x;
      m[i].m[1]= x;
      m[i].m[2]= x;
   }
} // setNM2

// Accumulate parallel sets of individually weighted moments of constant x
void accumNM2 (M2 m[], const WF w[], const WF x, const int n)
{
   WF x2= x * x;
   for (int i=0; i<n; i++)
   {
      m[i].m[0]+= w[i]; // x^0=1
      m[i].m[1]+= x * w[i];
      m[i].m[2]+= x2 * w[i];
   }
} // accumNM2

void scaleNF (WF r[], const WF x[], const int n, const WF k) { for (int i=0; i<n; i++) { r[i]= x[i] * k; } }

// Gaussian model functions
const WF K0= 1.0 / sqrt(2 * M_PI);
// Convert model descriptor to coefficients for efficient evaluation
void getNGK (GK gk[], const GM gm[], const int n)
{  // assume ((pGM->p > EPS) && (pGM->sd > EPS))
   for (int i=0; i<n; i++)
   {
      gk[i].k[0]= gm[i].p * K0 / gm[i].sd;
      gk[i].k[1]= gm[i].m;
      gk[i].k[2]= -1 / (2 * gm[i].sd * gm[i].sd);
   }
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

/* Initial Pattern Scanning */

int findPeaks (int r[], const int maxR, const WF f[], const int nF)
{
   int nR= 0;
   for (int i=1; i<(nF-1); i++)
   {
      if ((f[i] > f[i-1]) && (f[i] > f[i+1]))
      { 
         if (nR < maxR) { r[nR]= i; }
         nR++; 
      }
   }
   return(nR);
} // findPeaks

int lmuSetGM (GM *pGM, const WF f[], const int l, const int m, const int u)
{
   if ((u > m) && (m > l))
   {
      pGM->p=  sumNF(f+l, u-l);
      pGM->m=  m;
      pGM->sd= 0.25 * sqrt(u-l);
      return(1);
   }
   return(0);
} // lmuSetGM

int estGM (GM gm[], const int maxM, const WF f[], const int nF)
{
   int nM= 0, nK, k[8], l=0, u=nF-1;

   nK= findPeaks(k, 8, f ,nF);
   if ((nK > 0) && (maxM > 0))
   {
      int i, m= MIN(nK, maxM)-1;
      for (i=0; i<m; i++)
      {
         u= (k[i] + k[i+1]) / 2;
         nM+= lmuSetGM(gm+nM, f, l, k[i], u);
         l= u;
      }
      nM+= lmuSetGM(gm+nM, f, l, k[i], nF-1);
      {
         WF rt=0, t=0;
         for (int j=0; j<nM; j++) { t+= gm[j].p; }
         if (t > 0) { rt= 1.0 / t; }
         for (int j=0; j<nM; j++) { gm[j].p*= rt; }
      }
   }
   
   return(nM);
} // estGM

/**/

// All-in-one EM pass with minimal memory usage (vulnerable to error where binning is strongly non-uniform)
int em (const WorkCtx *pC, const GK gk[], const int nGK, const WF pmf[], const int nPMF)
{
   setNM2(pC->pM2, nGK, 0);
   for (int i= 0; i<nPMF; i++)
   {
      WF s= evalNGK(pC->pE, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(pC->pE, pC->pE, nGK, pmf[i] / s);
         // Accumulate as moments of order 0,1,2
         accumNM2(pC->pM2, pC->pE, i, nGK);
      }
   }
   // Convert moments to Gaussian model descriptors
   return setNGM(pC->pR, pC->pM2, nGK);
} // em

// Separate E,M passes requiring large [nM*nO] buffer for intermediate results
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
      {  // Classic 2-pass measurement of mean&variance: robust for arbitrary data
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

const WorkCtx * initWC (WorkCtx *pWC, const WF *pO, const size_t maxO, const size_t maxM, size_t maxE)
{
   if ((maxO > 0) && (maxM > 0) && (NULL == pWC->mb.p))
   {
      if (0 == maxE) { maxE= maxO * maxM; }
      pWC->mb.bytes= maxE * sizeof(WF) + maxM * (sizeof(GM) + sizeof(GK) + sizeof(M2));
      if (NULL == pO) { pWC->mb.bytes+= sizeof(WF) * maxO; }
      pWC->mb.bytes= alignPO2(pWC->mb.bytes, 12); // round up 4K
      pWC->mb.p= malloc(pWC->mb.bytes);
      if (pWC->mb.p)
      {
         if (NULL == pO)
         {  // Test mode, observations to be synthesised
            pWC->pO= pWC->mb.p;
            pWC->pR= (void*)(pWC->pO + maxO);
         }
         else
         {
            pWC->pO= pO;
            pWC->pR= pWC->mb.p;
         }
         pWC->pGK= (void*)(pWC->pR + maxM);
         pWC->pM2= (void*)(pWC->pGK + maxM);
         pWC->pE=  (void*)(pWC->pM2 + maxM);
         pWC->maxM= maxM;
         pWC->maxO= maxO;
         pWC->maxE= maxE;
         // Set extra workspace (for whatever..)
         pWC->ws.p= pWC->pE + maxE;
         pWC->ws.bytes= pWC->mb.bytes - (pWC->ws.w - pWC->mb.w);
         return(pWC);
      }
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
   WorkCtx wc={0,};
   const WorkCtx *pWC= initWC(&wc,NULL,32,2,0);

   if (pWC)
   {
      const GM gmm[2]={{0.2,6,1},{0.8,20,4}};
      genObs((WF*)(pWC->pO), pWC->maxO, gmm, 2);
      r= t2(pWC);
      freeWC(&wc);
   }
   return(r);
} // main

#endif
