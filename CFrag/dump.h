// dump.h - Vector data text dumping.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: GPL V3
// (c) Project Contributors July-Sept 2020

#ifndef DUMP_H
#define DUMP_H

#include "em.h"


/***/

void dumpNXB (const void *p, const int n);
void dumpHNXB (const void *p, const int n);
void dumpNF (const WF f[], const int n);
void dumpHNF (const WF f[], const int n);
void dumpHMNF (const WF f[], const int m, const int n);
void dumpINZNF (const WF f[], const int n);

#endif // DUMP_H
