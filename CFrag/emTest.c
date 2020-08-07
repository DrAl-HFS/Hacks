// emTest.c - Simple EM test code.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July-August 2020

#include "emTest.h"

WF uniformSampleGK (WF obs[], const WF x0, const WF dx, const int nObs, const GK gk[], const int nGM)
{
   WF t= 0, x= x0;
   for (int i= 0; i<nObs; i++)
   {
      WF ox= 0;
      for (int j= 0; j<nGM; j++)
      {
         const WF xm= x - gk[j].k[1];
         ox+= gk[j].k[0] * exp(gk[j].k[2] * xm * xm);
      }
      t+= obs[i]= ox;
      x+= dx;
   }
   return(t*dx); // Eulerian integral
} // uniformSampleGK

size_t getNoise (void *p, size_t bytes)
{
   size_t r= 0;
   if (p && bytes)
   {
      FILE *fd= fopen("/dev/random","r");
      if (fd)
      {
         r= fread(p, 1, bytes, fd);
         fclose(fd);
      }
   }
   return(r);
} // getNoise

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

void dumpNXB (const void *p, const int n)
{
   for (int i=0; i<n; i++) { printf("%02X", ((char*)p)[i]); }
} // dumpNXB

void dumpHNXB (const void *p, const int n)
{
   if (p && (n > 0))
   {
      printf("[%d]= ", n);
      dumpNXB(p,n);
      printf("\n");
   } else printf("[]\n");
} // dumpHNXB

void dumpNF (const WF f[], const int n)
{
   if (n > 0) printf("%G", f[0]);
   for (int i=1; i<n; i++) { printf(", %G",f[i]); }
} // dumpNF

void dumpHNF (const WF f[], const int n)
{
   if (n > 0)
   {
      printf("[%d]= ", n);
      dumpNF(f,n);
      printf("\n");
   } else printf("[]\n");
} // dumpHNF

void dumpHMNF (const WF f[], const int m, const int n)
{
   if ((m > 0) && (n > 0))
   {
      printf("[%d,%d]= ", m, n);
      printf("{");
      dumpNF(f,n);
      printf("}");
      for (int i=1; i<m; i++)
      {
         printf(", {");
         dumpNF(f+i*n,n);
         printf("}");
      }
      printf("\n");
   } else printf("[]\n");
} // dumpMNF

void dumpINZNF (const WF f[], const int n)
{
   if (n > 0)
   {
      for (int i=0; i<n; i++) { if (0 != f[i]) printf("[%d]= %G\n",i,f[i]); }
   } else printf("[]\n");
} // dumpINZNF

void t1 (void)
{
   int x[]={1,2,3,4,5}, n= 5;
   WF f[5]={-1};

   n= normP1NIF(f,x,n);
   dumpNF(f,n);
} // t1

#define MAX_GEN_GK 8
int genObs (WF obs[], int nObs, const GM gmm[], int nM, int verbose)
{
   GK gk[MAX_GEN_GK];

   if (nM > MAX_GEN_GK) { nM= MAX_GEN_GK; }
   getNGK(gk, gmm, nM);

   WF t= uniformSampleGK(obs, 0,1, nObs, gk, nM);
   if (verbose > 0)
   {
      printf("genObs() - synth");
      dumpHMNF((void*)gmm, nM,3);
      if (verbose > 1)
      {
         printf("Eval t=%G:\n", t);
         dumpINZNF(obs,nObs);
      }
   }
   return(nM);
} // genObs


int t2 (const WorkCtx *pWC, const int nP, int verbose)
{
   WF ss[2]={0,0};
   WF sad=-1, resid=-1;
   GM *pR= pWC->pR;
   int nM, aR=0, dR=0, nR=0;
   size_t bR;

   nM= estGM(pWC->pR, NULL, pWC->maxM, pWC->pO, pWC->maxO, 0.5);
   if (verbose > 0) { printf("estGM"); dumpHMNF((void*)(pWC->pR), nM, GM_NK); }

   bR= nM * nP * 2 * sizeof(GM);
   if (pWC->ws.bytes >= bR)
   {
      pR= pWC->ws.p;
      dR= nM;
   }
   if (verbose > 1) { printf("--E+M--\n"); }
   for (int i=0; i<nP; i++)
   {
      getNGK(pWC->pGK, pWC->pR, nM);
      expect(pWC->pE, pWC->pGK, nM, pWC->pO, pWC->maxO);
      maximise(pR+aR, pWC->pE, nM, pWC->maxO);
      if (verbose > 1) { printf("I%02d : mgm=", i); dumpHMNF((void*)(pR+aR), nM, GM_NK); }
      aR+= dR;
   }
   nR= aR - dR;
   if (verbose > 1) { printf("--EM--\nS"); }
   for (int i=0; i<nP; i++)
   {
      getNGK(pWC->pGK, pWC->pR, nM);
      em(pR+aR,pWC->pGK,nM,pWC);
      if (verbose > 1) { printf("I%02d : mgm=", i); dumpHMNF((void*)(pR+aR), nM, GM_NK); }
      aR+= dR;
   }
   if (nR > 0)
   { // compare
      int nF= nP*nM*GM_NK;
      WF ss[2]={0,0};
      diffNF(pWC->pE, (void*)pR, (void*)(pR+nR), nF); // HACK! reuse of expectation buffer
      sad= addSplitSumNF(ss, pWC->pE, nF);
      if (verbose > 0)
      {
         if (1 == verbose)
         {
            printf("E+M [%d] : ", nP-1); dumpHMNF((void*)(pR+nR), nM, GM_NK);
            printf("EM [%d] : ", nP-1); dumpHMNF((void*)(pR+aR-dR), nM, GM_NK);
         }
         printf("SS=[%G,%G], SAD=%G\n",ss[0],ss[1],sad);
      }
      pR+= nR;
   }
   genObs(pWC->pE, pWC->maxO, pR, nM, 0); // HACK! reuse of expectation buffer
   diffNF(pWC->pE, pWC->pO, pWC->pE, pWC->maxO);
   ss[0]= ss[1]= 0;
   resid= addSplitSumNF(ss, pWC->pE, pWC->maxO);
   printf("resid=%G [%G, %G]\n",resid, ss[0], ss[1]);
   return(sad < 1E-6);
} // t2
