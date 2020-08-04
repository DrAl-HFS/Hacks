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
   getNGK(gk, gmm, nM);
      
   WF t= uniformSampleGK(obs, 0,1, nObs, gk, nM);
   printf("genObs() - synth");
   dumpHMNF((void*)gmm, nM,3);
   printf("Eval t=%G:\n", t);
   dumpINZNF(obs,nObs);
   return(nM);
} // genObs


int t2 (const WorkCtx *pWC)
{
   int nM= estGM(pWC->pR, pWC->maxM, pWC->pO, pWC->maxO);

   printf("estGM");
   dumpHMNF((void*)(pWC->pR), nM,3);

   for (int i=0; i<10; i++)
   {
      getNGK(pWC->pGK, pWC->pR, nM);
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
