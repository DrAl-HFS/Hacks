// Hacks/CFrag/ElecTool/tabLCR.h - Arg handling for tuned circuit tabulator
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr 2019

#ifndef ARGS_LCR_H
#define ARGS_LCR_H

/*

 argsLCR.h - Args handling for tuned circuit tabulator

 (c) April 2019 DrAl-HFS <DrAl-HFS@github.com>

*/

#include "sciFmt.h"

/***/
/*
typedef unsigned char  U8;
typedef signed char    I8;
typedef unsigned short U16;
typedef signed short   I16;
typedef int Bool32;

#define TRUE 1
#define FALSE 0
*/
typedef struct { F32 min, max; } MMF;
typedef struct { U16 min, max; } MMU16;

typedef struct
{
   MMF c;
   U8 nStep;
} CapParam;

typedef struct
{
   F32 diam, len;
} Cylinder;

typedef struct
{
   Cylinder c;
   F32      perm;
   MMU16   wireTurns;
   U8       wireGuage, guageUnit, permUnit, pad[1];
} SolParam;

typedef struct
{
   CapParam cap;
   SolParam sol;
} Args;

/***/

void procArgs (Args *pA, const int argc, const char * const argv[]);

#endif // ARGS_LCR_H
