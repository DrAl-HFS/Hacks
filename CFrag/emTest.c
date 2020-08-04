// emTest.c - Simple EM test code.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July 2020

#include "stdio.h"

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
} // uniformSampleGM

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
int genObs (WF obs[], int nObs, const GM gmm[], int nM)
{
   GK gk[MAX_GEN_GK];
   
   if (nM > MAX_GEN_GK) { nM= MAX_GEN_GK; }
   for (int i= 0; i<nM; i++) { getGK(gk[i].k, gmm+i); }
   WF t= uniformSampleGK(obs, 0,1, nObs, gk, nM);
   printf("genObs() - synth");
   dumpHMNF((void*)gmm, nM,3);
   printf("Eval t=%G:\n", t);
   dumpINZNF(obs,nObs);
   return(nM);
} // genObs

#define MIN(a,b) ((a)<(b)?(a):(b))

int setGM (GM *pGM, const WF f[], const int l, const int m, const int u)
{
   if (u > l)
   {
      pGM->p=  sumNF(f+l, u-l);
      pGM->m=  m;
      pGM->sd= 0.25 * sqrt(u-l);
      return(1);
   }
   return(0);
} // setGM

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
         nM+= setGM(gm+nM, f, l, k[i], u);
         l= u;
      }
      nM+= setGM(gm+nM, f, l, k[i], nF-1);
      {
         WF rt=0, t=0;
         for (int j=0; j<nM; j++) { t+= gm[j].p; }
         if (t > 0) { rt= 1.0 / t; }
         for (int j=0; j<nM; j++) { gm[j].p*= rt; }
      }
   }
   
   return(nM);
} // estGM


int t2 (const WorkCtx *pWC)
{
   int nM= estGM(pWC->pR, pWC->maxM, pWC->pO, pWC->maxO);
   printf("estGM");
   dumpHMNF((void*)(pWC->pR), nM,3);
   //for (int j=0; j<nM; j++) { printf(" {%G, %G, %G}", pWC->pR[j].p, pWC->pR[j].m, pWC->pR[j].sd); }
   //printf("\n");
   //for (int j=0; j<nM; j++) { pWC->pR[j]= egm[j]; }

   //for (int j=0; j<nM; j++) { getGK(pWC->pGK[j].k, pWC->pR+j); } // pWC->pGK+3*j
   for (int i=0; i<10; i++)
   {
      for (int j=0; j<nM; j++) { getGK(pWC->pGK[j].k, pWC->pR+j); } // pWC->pGK+3*j
#if 1
      em(pWC,pWC->pGK,nM,pWC->pO,pWC->maxO);
#else
      expect(pWC->pE, pWC->pGK, nM, pWC->pO, pWC->maxO);
      maximise(pWC->pR, pWC->pE, nM, pWC->maxO);
#endif
      printf("I%02d : mgm=", i);
      dumpHMNF((void*)(pWC->pR), nM,3);
   }
   return(0);
} // t2
