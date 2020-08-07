// emTest.h - Interface for simple EM test code.
// https://github.com/DrAl-HFS/Hacks.git (GPL3 licence)
// (c) Project Contributors July-August 2020

#ifndef EM_TEST_H
#define EM_TEST_H

#include "dump.h"

extern void t1 (void);

//extern size_t getNoise (void *p, size_t bytes);
extern int getNoiseF (WF f[], const int n, const WF noiseMean, const WF noiseFrac, void *p, int verbose);

extern int genObs (WF obs[], int nObs, const GM gmm[], int nM, int verbose);

extern int t2 (const WorkCtx *pWC, const int nP, int verbose);

#endif // EM_TEST_H
