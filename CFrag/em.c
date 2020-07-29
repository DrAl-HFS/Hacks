
#include "math.h"


/**/

typedef double WF; // Wide float type
typedef struct { WF p,m,sd; } GM; // Gaussian (mixture) Model descriptor

// Pure float routines
// Vector->scalar reductions assume n>=1
//WF sumNF (const WF x[], const int n) { WF t= (n>0)?x[0]:0; for (int i=1; i<n; i++) { t+= x[i]; } return(t); }
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

WF dotNF (const WF x[], const WF y[], const int n) { WF t= x[0] * y[0]; for (int i=1; i<n; i++) { t+= x[i] * y[i]; } return(t); }

void scaleNF (WF r[], const WF x[], const int n, const WF k) { for (int i=0; i<n; i++) { r[i]= x[i] * k; } }

// Accumulate parallel sets of individually weighted moments of constant x
void accumM3NF (WF rm0[], WF rm1[], WF rm2[], const WF w[], const WF x, const int n)
{
   WF x2= x * x;
   for (int i=0; i<n; i++)
   { 
      rm0[i]+= w[i]; // x^0=1
      rm1[i]+= x * w[i];
      rm2[i]+= x2 * w[i];
   } 
} // accumM3NF


const WF K0= 1.0 / sqrt(2 * M_PI);

void getGK (WF gk[3], const GM *pGM)
{  // assume ((pGM->p > EPS) && (pGM->sd > EPS))
   gk[0]= pGM->p * K0 / pGM->sd;
   gk[1]= pGM->m;
   gk[2]= -1 / (2 * pGM->sd * pGM->sd);
} // getGK

WF evalNGK (WF p[], const WF x, const WF gk[][3], const int n) // gk=[kP,M,kV]
{
   WF t= 0;
   for (int i= 0; i<n; i++)
   {  // assume gk[i][0] > EPS
      const WF xm= x - gk[i][1];
      t+= p[i]= gk[i][0] * exp(gk[i][2] * xm * xm);
   }
   return(t);
} // evalGK

void expect (WF p[], const WF gk[][3], const int nGK, const WF pmf[], const int nPMF)
{
   for (int i= 0; i<nPMF; i++)
   {
      const int j= nGK*i;
      WF s= evalNGK(p+j, i, gk, nGK);
      if (s > 0)
      {  // Compute new partial probabilities weighted by observations
         scaleNF(p+j, p+j, nGK, pmf[i] / s);
         //accumM3NF(m0,m1,m2,p+j,i,nGK)
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
      {
         const WF rp= 1.0 / pR[i].p;
         pR[i].m= sumIProdStrideNF(p+i, nGK, nPMF) * rp;
         pR[i].sd= sqrt(sumISSDStrideNF(p+i, nGK, nPMF, pR[i].m) * rp);
         nK++;
      }
   }
   // if (nK < nGK) remove degenerates
   return(nK);
} // maximise

// int to float routines
WF sumNIF (const int x[], const int n) { WF s= x[0]; for (int i=1; i<n; i++) { s+= x[i]; } return(s); }

void scaleNIF (WF r[], const int x[], const int n, const WF s) { for (int i=0; i<n; i++) { r[i]= s * x[i]; } }

int normP1NIF (WF r[], const int x[], const int n)
{
   WF s= sumNIF(x,n);
   if (0 != s) { scaleNIF(r,x,n,1.0/s); return(n); }
   return(0);
} // normP1NIF


#ifndef LIB_TARGET

#include "stdio.h"

#define MAX_GM 1

WF uniformSampleGMM (WF p[], const WF x0, const WF dx, const int n, const GM gmm[], const int nGM)
{
   WF gk[MAX_GM][3];
   WF t= 0, x= x0;
   //for (int j= 0; j<nGM; j++) {
   getGK(gk[0], gmm+0);
   for (int i= 0; i<n; i++)
   {
      //for (int j= 0; j<nGM; j++) {
      const WF xm= x - gk[0][1];
      t+= p[i]= gk[0][0] * exp(gk[0][2] * xm * xm);
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

#define NP 32
#define NM 2
void t2 (void)
{
   WF p[NP]={0,};
   GM gm={1,16,4};
   WF t= uniformSampleGMM(p, 0,1, 32, &gm, 1);
   printf("uniformSampleGMM() - t=%G: ", t);
   dumpINZNF(p,32);
   {
      const GM egm[NM]={{0.3,5,1},{0.6,22,16}};
      GM mgm[NM];
      WF gk[NM][3];
      WF e[NP*NM];
            
      for (int j=0; j<NM; j++) { mgm[j]= egm[j]; }
      for (int i=0; i<10; i++)
      {
          for (int j=0; j<NM; j++) { getGK(gk[j], mgm+j); }
          expect(e, gk, NM, p, NP);
          maximise(mgm, e, NM, NP); 
          for (int j=0; j<NM; j++) { getGK(gk[j], mgm+j); }
          printf("i%d : mgm[]=", i);
          for (int j=0; j<NM; j++) { printf(" {%G, %G, %G}", mgm[j].p, mgm[j].m, mgm[j].sd); }
          printf("\n");
      }
   }
} // t2

int main (int argc, char *argv[]) { t2(); return(0); }

#endif
