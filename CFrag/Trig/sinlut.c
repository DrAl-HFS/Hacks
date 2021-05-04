#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef float Scalar;

typedef Scalar (*ScalFP) (Scalar);
typedef Scalar (*IvlFP) (Scalar,Scalar);

Scalar ivlSinAvg (const Scalar x0, const Scalar x1)
{
   const int n=8;
   const Scalar dx= (x1-x0)/(n-1);
   Scalar x=x0, s= sin(x);
   for (int i=1; i<n; i++) { x+= dx; s+= sin(x); }
   return(s/n);
} // ivlSinAvg

int genf1dx (Scalar r[], const int n, Scalar x, const Scalar dx, ScalFP func1)
{
   for (int i=0; i<n; i++) { r[i]= func1(x); x+= dx; }
   return(n);
} // genfdx

int genf2dx (Scalar r[], const int n, Scalar x, const Scalar dx, IvlFP func2)
{
   for (int i=0; i<n; i++) { r[i]= func2(x,x+dx); x+= dx; }
   return(n);
} // genfdx

void genLutX (const Scalar x0, const Scalar dx, const int n, const int kX)
{
   Scalar *pS= malloc(n * sizeof(*pS));
   if (pS)
   {
      genf1dx(pS, n, x0, dx, sinf);
      //genf2dx(pS, n, x0, dx, ivlSinAvg);
      for (int i=0; i<n; i++)
      {
         int sX= 0.5 + pS[i] * kX; // round up fixed point
         printf("0x%02X, ", sX);
      }
      free(pS);
   }
} // genLutX

#if 0
void precTest (void)
{
   double x= M_PI, dx= 1E-15;
   double r;

   for (int i= -20; i<=20; i++)
   {
      x= M_PI + i * dx;
      r= sin(x);
      printf("%d: sin(%.12G)= %.12G\n", i, x, r);
   }
} // precTest
#endif

void genDomX (const Scalar dom[2], const int n, const int kX)
{
   const Scalar dx= (dom[1] - dom[0]) / (n-1); // n-1 intervals
   genLutX(dom[0], dx, n, 0x7F);
} // genDomX

void genOffsDomX (const Scalar dom[2], const int n, const int kX)
{
   const Scalar offset= (dom[1] - dom[0]) / (10*n); // deliberate skew
   const Scalar oDom[2]= {dom[0]+offset, dom[1]-offset};

   genDomX(oDom, n, kX);
} // genOffsDomX

int main (int argc, char *argv[])
{
   const int n= 16;
   const Scalar dom[2]={ 0, 0.5*M_PI };

   genOffsDomX(dom, n, 0x7F);
   //genDomX(dom,n,0x7F);
} // main
