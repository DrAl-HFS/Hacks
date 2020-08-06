// em.c - Simple 1D Expectation Maximisation for distribution modelling.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July-August 2020

#include "em.h"


/***/

int validMB (const MB *pMB, const size_t bytes) { return (pMB && pMB->p && (pMB->bytes >= bytes)); }

// Float vector functions 
void diffNF (WF d[], const WF x[], const WF y[], const int n) { for (int i=0; i<n; i++) { d[i]= x[i] - y[i]; } }
void scaleNF (WF s[], const WF x[], const int n, const WF k) { for (int i=0; i<n; i++) { s[i]= x[i] * k; } }
void prodNF (WF r[], const WF x[], const WF y[], const int n) { for (int i=0; i<n; i++) { r[i]= x[i] * y[i]; } }

// scalar reductions
WF sumNF (const WF x[], const int n) { WF t= (n>0)?x[0]:0; for (int i=1; i<n; i++) { t+= x[i]; } return(t); }
// Summation by sign [-ve,+ve] returns absolute sum
WF addSplitSumNF (WF ss[2], const WF x[], const int n) { for (int i=0; i<n; i++) { ss[ (x[i] >= 0) ] += x[i]; } return(ss[1] - ss[0]); }
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

void zeroNM2 (M2 m[], const size_t n) { memset(m, 0, n * sizeof(M2)); }
/*void setNM2 (M2 m[], const int n, const WF x)
{
   for (int i=0; i<n; i++)
   {
      m[i].m[0]= x;
      m[i].m[1]= x;
      m[i].m[2]= x;
   }
} // setNM2
*/

/***/

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

WF lerp (WF a, WF b, WF r) { return(a + r * (b-a)); }

// Remove smaller peaks, preserving order
int trimPeaks (int *pI, int nI, const int nR, const WF f[])
{  // Flaky magic numbers... TODO: use sort / dist to find n most significant
   while (nI > nR)
   {
      WF r, u, v, vMax, tf;
      int n= 0;
      vMax= tf= f[ pI[0] ];
      for (int i=1; i<nI; i++) 
      {  // get total and max
         v= f[ pI[i] ];
         if (v > vMax) { vMax= v; }
         tf+= v;
      }
      // Estimate suitable threshold
      u= tf / nI; // lower bound (mean)
      r= (WF)(nI - nR) / nI; // Fraction to remove, combine with constant for non-linear
      //r-= 0.5; // assume 50% ~ uniform
      if (vMax > u) { v= lerp(u, vMax, 1.0/16 * r); } else { v= u; }
      printf("trimPeaks() - u=%G, vMax=%G, r=%G, t=%G\n", u, vMax, r, v);
      // Swap down those above threshold
      for (int i=1; i<nI; i++) 
      {  // get total and max
         if (f[ pI[i] ] >= v)
         {
            SWAP(int, pI[n], pI[i]);
            ++n;
         }
      }
      nI= n;
   } 
   return(nI);
} // trimPeaks

int lmuSetGM (GM *pGM, const WF f[], const int l, const int m, const int u)
{
   M2 m2={{0,0,0}};
   for (int i=l; i<u; i++)
   {
      m2.m[0]+= f[i]; // x^0=1
      m2.m[1]+= i * f[i];
      m2.m[2]+= i*i * f[i];
   }
   return setNGM(pGM, &m2,1);
#if 0
   if ((u > m) && (m > l))
   {
      pGM->p=  sumNF(f+l, u-l);
      pGM->m=  m;
      pGM->sd= 0.25 * sqrt(u-l);
      return(1);
   }
   return(0);
#endif
} // lmuSetGM

int estGM (GM gm[], int *pI, const int maxM, const WF f[], const int nF)
{
   int nI, maxI=maxM, nM= 0, l=0, u=nF-1;
   void *p=NULL;
   
   if (NULL == pI)
   {
      maxI= nF / 3; // worst case?
      pI= p= malloc(sizeof(*pI) * maxI); 
   }
   nI= findPeaks(pI, maxI, f ,nF);
   if (nI > maxM)
   {
      printf("WARNING: [em] estGM() - nI=%d\n", nI);
      nI= trimPeaks(pI, nI, maxM, f);
   }
   if ((nI > 0) && (maxM > 0))
   {
      int i, n= MIN(nI, maxM)-1;
      for (i=0; i<n; i++)
      {
         u= (pI[i] + pI[i+1]) / 2;
         nM+= lmuSetGM(gm+nM, f, l, pI[i], u);
         l= u;
      }
      if ((nM < maxM) && (i<nI))
      {
         nM+= lmuSetGM(gm+nM, f, l, pI[i], nF-1);
      }
      {  // normalise
         WF rt=0, t=0;
         for (int j=0; j<nM; j++) { t+= gm[j].p; }
         if (t > 0) { rt= 1.0 / t; }
         for (int j=0; j<nM; j++) { gm[j].p*= rt; }
      }
   }
   if (p) { free(p); }
   return(nM);
} // estGM

/**/

// All-in-one EM pass with minimal memory usage (vulnerable to error where binning is strongly non-uniform)
int em (GM rgm[], const GK gk[], const int nGK, const WorkCtx *pC)
{
   zeroNM2(pC->pM2, nGK);
   for (int i= 0; i < pC->maxO; i++)
   {
      WF s= evalNGK(pC->pE, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(pC->pE, pC->pE, nGK, pC->pO[i] / s);
         // Accumulate as moments of order 0,1,2
         accumNM2(pC->pM2, pC->pE, i, nGK);
      }
   }
   // Convert moments to Gaussian model descriptors
   return setNGM(rgm, pC->pM2, nGK);
} // em

// Separate E,M passes requiring large [nM*nO] buffer for intermediate results
void expect (WF e[], const GK gk[], const int nGK, const WF pmf[], const int nPMF)
{
   for (int i= 0; i<nPMF; i++)
   {
      const int j= nGK*i;
      WF s= evalNGK(e+j, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(e+j, e+j, nGK, pmf[i] / s);
      }
   }
} // expect

int maximise (GM rgm[], const WF e[], const int nGK, const int nPMF)
{
   int nK= 0;
   for (int i= 0; i<nGK; i++)
   {
      rgm[i].p= sumStrideNF(e+i, nGK, nPMF);
      if (rgm[i].p > 0)
      {  // Classic 2-pass measurement of mean then variance: robust for arbitrary data
         const WF rp= 1.0 / rgm[i].p;
         rgm[i].m= sumIProdStrideNF(e+i, nGK, nPMF) * rp;
         rgm[i].sd= sqrt(sumISSDStrideNF(e+i, nGK, nPMF, rgm[i].m) * rp);
         nK++;
      }
   }
   return(nK);
} // maximise

// TODO: if (nK < nGK) remove degenerates

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

int em1DNF (GM *pR, const int maxR, const WF obs[], const int nObs, const int mf)
{
   //printf("em1DNF(%p, %d, %p, %d, %d)\n",pR,maxR,obs,nObs,mf);
   int nM= estGM(pR, NULL, maxR, obs, nObs);
   if (nM > 0)
   {
      const int maxIter= mf & 0xFF;
      const U8 f= mf >> 16;
      WorkCtx wc={0,};
      
      if (maxIter > 0)
      {
         initWC(&wc, obs, nObs, nM, nM*(0==(f & 0x80)) );
         
         if (nM <= maxR) { memcpy(wc.pR, pR, nM * sizeof(*pR)); }
         else { nM= estGM(wc.pR, wc.ws.p, wc.maxM, obs, nObs); } // run again with more space
         
         for (int i=0; i<maxIter; i++)
         {
            getNGK(wc.pGK, wc.pR, nM);
            em(wc.pR, wc.pGK, nM, &wc);
            //if (verbose > 1) { printf("I%02d : mgm=", i); dumpHMNF((void*)(pR+aR), nM, GM_NK); }
         }
         freeWC(&wc);
      }
   }
   return(nM);
} // em1DNF

#ifndef LIB_TARGET

#include "emTest.h"

void linMapNBF (WF f[], const U8 b[], const int n, const WF lm[2]) 
{
   for (int i=0; i<n; i++) { f[i]= lm[0] + b[i] * lm[1]; }
} // linMapNBF

unsigned long sumU8 (U8 u[], const int n) { unsigned long t= (n>0)?u[0]:0; for (int i=0; i<n; i++) { t+= u[i]; } return(t); }

int getNoiseF (WF f[], const int n, const WF noiseMean, const WF noiseFrac, void *p, int verbose)
{
   int r= getNoise(p,n);
   if (r > 0)
   {
      WF lm[2], nm=0;
      
      nm= (WF)sumU8(p,r) / r;
      lm[1]= noiseFrac / (0.5 * 0xFF);
      lm[0]= noiseMean - 0.5*noiseFrac - 0.475*nm*lm[1];
      if (verbose > 1) { printf("getNoise() - "); dumpHNXB(p, r); printf("mean= %G\n", nm); }
      linMapNBF(f, p, r, lm);
      
      if (verbose > 1)
      {
         WF t, m;
         t= sumNF(f,r);
         m= t / r;
         printf("noise modulation "); dumpHNF(f, r);
         printf("sum= %G, mean= %G, err= %G\n", t, m, noiseMean-m);
      }
   }
   return(r);
} // getNoiseF

int main (int argc, char *argv[])
{
   int verbose=2, r= 0;
   WorkCtx wc={0,};
   const WorkCtx *pWC= initWC(&wc,NULL,32,2,0);

   if (pWC)
   {
      const GM gmm[2]={{0.2,6,1},{0.8,20,4}};
      genObs((WF*)(pWC->pO), pWC->maxO, gmm, 2, verbose); // synthesise
      if (0)
      {  // noise modulation
         r= getNoiseF(pWC->pE, MIN(pWC->ws.bytes,pWC->maxO), 1.0, 1.0/32, pWC->ws.p, verbose);
         if (r > 0) { prodNF((WF*)(pWC->pO), pWC->pO, pWC->pE, MIN(r, pWC->maxO)); }
      }
      r= t2(pWC,10,verbose); // test
      freeWC(&wc);
   }
   return(r);
} // main

#endif
