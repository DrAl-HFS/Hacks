// emTest.c - Simple EM test code.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July 2020

#include "stdio.h"

#define MAX_OBS 32
#define MAX_GM 2

WF uniformSampleGMM (WF obs[], const WF x0, const WF dx, const int nObs, const GM gmm[], const int nGM)
{
   WF gk[MAX_GM][3];
   WF t= 0, x= x0;
   for (int j= 0; j<nGM; j++) { getGK(gk[j], gmm+j); }
   for (int i= 0; i<nObs; i++)
   {
      WF ox= 0;
      for (int j= 0; j<nGM; j++)
      {
         const WF xm= x - gk[j][1];
         ox+= gk[j][0] * exp(gk[j][2] * xm * xm);
      }
      t+= obs[i]= ox;
      x+= dx;
   }
   return(t*dx); // Eulerian integral
} // uniformSampleGM

void dumpNF (const WF f[], const int n)
{
   if (n > 0)
   {
      printf("[%d]= %G", n, f[0]);
      for (int i=1; i<n; i++) { printf(", %G",f[i]); }
      printf("\n");
   } else printf("[]\n");
} // dumpNF

void dumpINZNF (const WF f[], const int n)
{
   if (n > 0)
   {
      for (int i=1; i<n; i++) { if (0 != f[i]) printf("[%d]= %G\n",i,f[i]); }
   } else printf("[]\n");
} // dumpINZNF

void t1 (void)
{
   int x[]={1,2,3,4,5}, n= 5;
   WF f[5]={-1};

   n= normP1NIF(f,x,n);
   dumpNF(f,n);
} // t1

void genObs (WF obs[], int nObs, int nM)
{
   const GM gmm[]={{0.2,6,1},{0.8,20,4}};
   WF t= uniformSampleGMM(obs, 0,1, nObs, gmm, nM);
   printf("genObs() - t=%G:\n", t);
   dumpINZNF(obs,nObs);
} // genObs

#define MIN(a,b) ((a)<(b)?(a):(b))

int t2 (const WorkCtx *pWC)
{
   const GM egm[]={{0.3,5,1},{0.6,22,16}};
   //GM mgm[MAX_GM];
   WF gk[MAX_GM][3];
   //WF e[MAX_OBS*MAX_GM];
   int nM= MIN(pWC->maxM, MAX_GM);
   int nO= MIN(pWC->maxO, MAX_OBS);
   
   for (int j=0; j<nM; j++) { pWC->pR[j]= egm[j]; }
   
   for (int j=0; j<nM; j++) { getGK(gk[j], pWC->pR+j); } // pWC->pGK+3*j
   for (int i=0; i<10; i++)
   {
       expect(pWC->pE, gk, nM, pWC->pO, nO);
       maximise(pWC->pR, pWC->pE, nM, nO);
       
       for (int j=0; j<nM; j++) { getGK(gk[j], pWC->pR+j); }
       printf("i%d : mgm[]=", i);
       for (int j=0; j<nM; j++) { printf(" {%G, %G, %G}", pWC->pR[j].p, pWC->pR[j].m, pWC->pR[j].sd); }
       printf("\n");
   }
   return(0);
} // t2
