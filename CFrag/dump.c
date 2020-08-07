// dump.c - Vector data text dumping.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July-August 2020

#include "dump.h"

/***/

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
