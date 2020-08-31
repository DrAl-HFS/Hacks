// em.c - Simple 1D Expectation Maximisation for distribution modelling.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: GPL V3
// (c) Project Contributors July-Sept 2020

#include "em.h"

/***/



/***/

int validMB (const MB *pMB, const size_t bytes) { return (pMB && pMB->p && (pMB->bytes >= bytes)); }

// Floating point data processing routines

// Vector routines (unstrided)
void diffNF (WF d[], const WF x[], const WF y[], const int n) { for (int i=0; i<n; i++) { d[i]= x[i] - y[i]; } }
void scaleNF (WF s[], const WF x[], const int n, const WF k) { for (int i=0; i<n; i++) { s[i]= x[i] * k; } }
void prodNF (WF r[], const WF x[], const WF y[], const int n) { for (int i=0; i<n; i++) { r[i]= x[i] * y[i]; } }
void sumProdNF (WF r[], const WF x[], const WF y[], const int n) { for (int i=0; i<n; i++) { r[i]= x[i] * y[i]; } }
int convNF (WF r[], const WF x[], const int nX, const WF k[], const int nK, const int mode)
{  // Discrete convolution of signal X with kernel K
   if ((nX > nK) && (nK > 1))
   {
      if (0 == mode)
      {
         for (int i= 0; i < (nX-nK); i++) { r[i]= dotNF(x+i, k, nK); }
         return(nX-nK);
      } //else
      for (int i= 0; i < (nX-nK); i++) { r[i+nK/2]= dotNF(x+i, k, nK); }
      switch (mode)
      {
         case 1 : // boundaries (partial kernel)
            for (int i= 1; i < nK; i++)
            {
               r[nK-i]= dotNF(x, k+i, nK-i);       // lo
               r[nX-i]= dotNF(x+nX-nK+i, k, nK-i); // hi
            }
            return(nX);
         case 2 : // periodic boundaries (wrapped kernel)
            for (int i= 1; i < nK; i++)
            {  // ???
               r[nK-i]= dotNF(x, k+i, nK-i) + dotNF(x+nX-i, k+nK-i, i);    // lo1+hi2
               r[nX-i]= dotNF(x+nX-nK+i, k, nK-i) + dotNF(x, k+i, nK-i);   // hi1+lo2
            }
            return(nX);
      }
   }
   return(0);
} // convNF

// Scalar reduction functions
WF sumNF (const WF x[], const int n) { WF t= (n>0)?x[0]:0; for (int i=1; i<n; i++) { t+= x[i]; } return(t); }
// Summation by sign [-ve,+ve] returns absolute sum
WF addSplitSumNF (WF ss[2], const WF x[], const int n) { for (int i=0; i<n; i++) { ss[ (x[i] >= 0) ] += x[i]; } return(ss[1] - ss[0]); }
WF dotNF (const WF x[], const WF y[], const int n) { WF t= (n>0)?x[0]*y[0]:0; for (int i=1; i<n; i++) { t+= x[i] * y[i]; } return(t); }

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

int sadMNF (WF sad[], const WF x[], const WF y[], const int m, const int n)
{
   int k= 0;
   for (int j= 0; j < m; j++) { sad[j]= 0; }
   for (int i= 0; i < n; i++)
   {
      for (int j= 0; j < m; j++)
      {
         sad[j]+= fabs(x[k] - y[k]);
         ++k;
      }
   }
   return(m);
} // sadMNF

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

int mmIdxNF (WF mm[], const int idx[], const int nIdx, const WF f[])
{
   if (nIdx > 0)
   {
      mm[0]= mm[1]= f[ idx[0] ];
      for (int i= 1; i<nIdx; i++)
      {
         WF v= f[ idx[i] ];
         if (v < mm[0]) { mm[0]= v; }
         if (v > mm[1]) { mm[1]= v; }
      }
      return(1 + (mm[1] > mm[0]));
   }
   return(0);
} // mmIdxNF

#define TRIM_BINS 64
// Remove smaller peaks, preserving order
int trimPeaks (int *pI, int nI, const int nR, const WF f[])
{
   WF mm[2], lm[2], t;

   if (2 == mmIdxNF(mm, pI, nI, f))
   {
      U8 dist[TRIM_BINS]={0,};
      const int nL= nR - nR/3; // nH=nR
      int iB, nT;

      lm[1]= TRIM_BINS / (mm[1] - mm[0]);
      lm[0]= - mm[0] * lm[1];
      for (int i= 1; i<nI; i++)
      {
         iB= f[ pI[i] ] * lm[1] + lm[0]; // map to bin
         dist[iB]++;
      }

      // find threshold
      iB= TRIM_BINS;
      nT= 0;
      while (nT < nL) { nT+= dist[--iB]; }
      t= (iB / lm[1]) + mm[0]; // inverse map
/*
      do // Check & adjust
      {
         nT= 0;
         for (int i=1; i<nI; i++) { nT+= (f[ pI[i] ] >= t); }
         if (nT < nL)
         {
            t= (--iB / lm[1]) + mm[0]; // reduce threshold to increase number
         }
      } while ((nT < nL) && (iB > 0));
*/
      if (nT >= nL)
      {  // Commit
         nT= 0;
         for (int i=1; i<nI; i++)
         {  // get total and max
            if (f[ pI[i] ] >= t)
            {
               SWAP(int, pI[nT], pI[i]);
               ++nT;
            }
         }
         nI= nT;
      }
   }
   return(nI);
} // trimPeaks

int lmuSetGM (GM *pGM, const WF f[], int l, int m, int u, const WF w)
{
   M2 m2={{0,0,0}};
   if (0 != w)
   {
      l= lerp(m,l,w);
      u= lerp(m,u,w);
   }
   for (int i=l; i<=u; i++)
   {
      m2.m[0]+= f[i]; // x^0=1
      m2.m[1]+= i * f[i];
      m2.m[2]+= i*i * f[i];
   }
   return setNGM(pGM, &m2, 1);
} // lmuSetGM

WF getRWP (const EEP *pE) { if (pE) return(pE->rwp); else return(0); }
int getTM (const int maxM, const EEP *pE) { if (pE && pE->tM > 0) return MIN(maxM, (int)(pE->tM)); else return(maxM); }

int estGM (GM gm[], const int maxM, const WF f[], const int nF, const EEP *pE)
{
   int nI, tM, nM= 0, l=0, u=nF-1;
   int maxI, *pI=NULL;

   if (NULL == pI)
   {
      maxI= nF / 3; // worst case?
      pI= malloc(sizeof(*pI) * maxI);
   }
   if (pI)
   {
      tM= getTM(maxM, pE);
      nI= findPeaks(pI, maxI, f ,nF);
      if (nI > tM)
      {
         printf("WARNING: [em] estGM() - nI=%d\n", nI);
         nI= trimPeaks(pI, nI, tM, f);
      }
      if ((nI > 0) && (tM > 0))
      {
         int i, n= MIN(nI, tM)-1;
         for (i=0; i<n; i++)
         {
            u= (pI[i] + pI[i+1]) / 2;
            nM+= lmuSetGM(gm+nM, f, l, pI[i], u, getRWP(pE));
            l= u;
         }
         if ((nM < tM) && (i<nI))
         {
            nM+= lmuSetGM(gm+nM, f, l, pI[i], nF-1, getRWP(pE));
         }
         {  // normalise
            WF rt=0, t=0;
            for (int j=0; j<nM; j++) { t+= gm[j].p; }
            if (t > 0) { rt= 1.0 / t; }
            for (int j=0; j<nM; j++) { gm[j].p*= rt; }
         }
      } else { printf("WARNING: [em] estGM() - nI=%d, tM=%d, maxM=%d\n", nI, tM, maxM); }
      free(pI);
   }
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
            pWC->pR[0]= (void*)(pWC->pO + maxO);
         }
         else
         {
            pWC->pO= pO;
            pWC->pR[0]= pWC->mb.p;
         }
         pWC->pR[1]= NULL;
         pWC->pGK= (void*)(pWC->pR[0] + maxM);
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

#include "dump.h"

const EEP *getExt (EEP *pE, const int mf)
{
   if (pE)
   {
      pE->flags= (mf >> 24) & 0xFF;
      pE->verbosity= pE->flags & 0x3;
      pE->maxIter= mf & 0xFF;
      pE->tM= (mf >> 8) & 0xFF;
      pE->rwp= ((mf >> 16) & 0xF0) * (1.0/128);
      pE->termSADR= pow(0.5, (mf >> 16) & 0x0F);
   }
   return(pE);
} // getExt

size_t getNE (const int nM, const EEP *pE) { if (pE->flags & FLAG_FLEM) return(0); else return(nM); }

const GM *setRef (WorkCtx *pWC, GM *pR, int nM, const EEP *pE)
{
   GM *pRef=NULL;
   if (pR && (nM > 0))
   {
      if ((NULL == pE) || (0 == (pE->flags & FLAG_FLIT)))
      {
         if (pWC && (pWC->maxM >= 2 * nM))
         {
            pRef= pWC->pR[0] + nM;
            memcpy(pRef, pR, nM * sizeof(*pR)); // copy estimate for later reference
         }
      }
   }
   return(pRef);
} // setRef

WF maxRNF (const WF x[], const WF y[], const int n)
{
   WF max= -1;
   for (int i=0; i<n; i++)
   {
      if (0 != y[i])
      {
         WF r= x[i] / y[i];
         if (r > max) max= r;
      }
   }
   return(max);
} // maxRNF

int em1DNF (GM *pR, const int maxR, const WF obs[], const int nObs, const int mf)
{
   WorkCtx wc={0,};
   EEP ext;
   int nM;

   nM= estGM(pR, maxR, obs, nObs, getExt(&ext,mf));
   if (nM > 0)
   {
      if (ext.verbosity > 1) { printf("em1DNF() - maxR=%d, nM=%d, tM=%d, maxIter=%d, termSADR=%G\n", maxR, nM, ext.tM, ext.maxIter, ext.termSADR); }
      if (ext.verbosity > 2) { printf("est:"); dumpHMNF((void*)pR, nM, GM_NK); }

      if ( (ext.maxIter > 0) && initWC(&wc, obs, nObs, 2*nM, getNE(nM,&ext)) )
      {
         const GM *pRef= setRef(&wc, pR, nM, &ext);
         U8 iter= 0, iDest=0, nextIter= 1;
         if (pRef)
         {
            getNGK(wc.pGK, pRef, nM);
            em(wc.pR[0], wc.pGK, nM, &wc);
            sadMNF(ext.sad[0], (WF*)(wc.pR[0]), (WF*)pRef, GM_NK, nM);
            if (ext.verbosity > 2) { printf("SAD:"); dumpHNF(ext.sad[0], GM_NK); }
            ++iter;
         }
         wc.pR[1]= pR; // iterate between internal and client buffer
         do
         {
            iDest= iter & 1;
            getNGK(wc.pGK, wc.pR[iDest^1], nM);
            em(wc.pR[iDest], wc.pGK, nM, &wc);
            if (pRef)
            {
               sadMNF(ext.sad[1], (WF*)(wc.pR[iDest]), (WF*)pRef, GM_NK, nM);
               WF mR= maxRNF(ext.sad[0], ext.sad[1], GM_NK);
               if (ext.verbosity > 1) { printf("maxRNF() - %G\n", mR); }
               nextIter= maxR > ext.termSADR;
            }
            if (ext.verbosity > 2) { printf("I%02d : mgm=", iter); dumpHMNF((void*)(wc.pR[iDest]), nM, GM_NK); }
         } while ((++iter < ext.maxIter) && nextIter);
         if (ext.verbosity > 1) { printf("em1DNF() - %d iterations\n", iter); }

         if (pR != wc.pR[iDest]) { memcpy(pR, wc.pR[iDest], nM * sizeof(*pR)); }
         freeWC(&wc);
      }
      return(nM);
   }
   return(0);
} // em1DNF

#ifndef LIB_TARGET

#include "emTest.h"

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
