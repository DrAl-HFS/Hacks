// Hacks/CFrag/ElecTool/tabLCR.h - Arg handling for tuned circuit tabulator
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr 2019

/*

 argsLCR.c - Args handling for tuned circuit tabulator

 (c) April 2019 DrAl-HFS <DrAl-HFS@github.com>

*/

#include <ctype.h>
#include "argsLCR.h"

/***/


int skipCharSetN (const char s[], const int maxS, const char t[], const int maxT)
{
   int n= 0;
   while ((n < maxS) && s[n])
   {
      int i= 0;
      while ((i < maxT) && t[i] && (s[n] != t[i])) { ++i; }
      if (s[n] != t[i]) { return(n); }
      // else
      ++n;
   }
   return(n);
} // skipCharSetN

void procArgs (Args *pA, const int argc, const char * const argv[])
{
   int i= 0;
   while (++i < argc)
   {
      const char *pCh= argv[i];

      if ('-' == pCh[0])
      {
         int n=2;
         switch(toupper(pCh[1]))
         {
            case 'C' :
               n+= sciFmtScanF32(&(pA->cap.c.min), pCh+n, 8);
               // skip sep
               n+= skipCharSetN(pCh+n, 8, ",;:", 3);
               n+= sciFmtScanF32(&(pA->cap.c.max), pCh+n, 8);
               n+= skipCharSetN(pCh+n, 8, ",;:", 3);
               break;
         }
      }
   }
} // procArgs
