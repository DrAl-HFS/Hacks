// Hacks/CFrag/ElecTool/tabLCR.c - Tuned circuit tabulator
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr 2019

/*

 tabLCR.c - Tuned circuit tabulator

 (c) April 2019 DrAl-HFS <DrAl-HFS@github.com>

*/

#include <string.h>
#include "argsLCR.h"

//#pragma clang diagnostic ignored "-Wformat-security"


/***/

typedef struct
{
	//const char *name;
	float resistivity, permeability; //, permittivity treated as infinite for pure metals
} MatProp;

typedef struct
{
	float	* p;
	size_t	n;
} FTab;

typedef struct
{
	FTab	ft;
	const char	* unit; // Volt Amp Ohm Farad Henry
	const char	* label;
} QTab;

typedef float (*F1V1FuncPtr) (float,void*);
typedef float (*F2V0FuncPtr) (float,float);

typedef struct
{
   F1V1FuncPtr pF1V1;
   void        *p;
   const char *unit;
   const char *label;
} F1V1Desc;

typedef struct
{
   F2V0FuncPtr pF2V0;
   const char *unit;
   const char *label;
} F2V0Desc;

typedef struct
{
   float uAdS;
} SolenoidParam;


/***/

const MatProp vacuum={ 1E20, M_PI * 4E-7 };
const MatProp Cu={1.72E-8,1.256629E-6};

//const float gMagPermFreeSpace= M_PI * 4E-7;
Args gArgs=
{
   { {50E-12, 120E-12}, 15 },
   {  }
};

/***/


Bool32 setFTab (FTab * const pFT, const size_t n)
{
	if (pFT && (NULL == pFT->p)) // 0 == pFT->n
	{
		if (0 == n)
		{
			pFT->n= 0;
			if (pFT->p) { free(pFT->p); }
			pFT->p= NULL;
		}
		else //if (n > 0)
		{
			pFT->p= malloc(n * sizeof(*(pFT->p)));
			if (pFT->p)
			{
				pFT->n= n;
				return(TRUE);
			}
		}
	}
	return(FALSE);
} // setFTab

void setSS (float f[], const size_t n, float v, const float s)
{
	size_t i;
	for (i= 0; i<n; i++) { f[i]= v; v+= s; }
} // setSS

Bool32 defFTabSS (FTab * const pFT, const size_t n, const float start, const float step)
{
	if ((n > 0) && setFTab(pFT, n))
	{
		setSS(pFT->p, pFT->n, start, step);
		return(TRUE);
	}
	return(FALSE);
} // defFTabSS

Bool32 defFTabLL (FTab * const pFT, const size_t n, const float vf, const float vl)
{
	if ((n > 0) && setFTab(pFT, n))
	{
		float r= 1;
		if (n > 2) { r/= (n-1); }
		setSS(pFT->p, pFT->n, vf, (vl - vf) * r );
		return(TRUE);
	}
	return(FALSE);
} // defFTabLL

//defQTab (QTab *pQ,


float diamSWG (U8 n)
{
static const float	SWG0_50[]=
{
	8.230, 7.620, 7.010, 6.401, // 0..3
	5.893, 5.385, 4.877, 4.470, // 4..7
	4.064, 3.658, 3.251, 2.946, // 8..11
	2.642, 2.337, 2.032, 1.829,	// 12..15
	1.626, 1.422, 1.219, 1.016,	// 16..19
	0.914, 0.813, 0.711, // 20..22  (0.004"/gauge)
	0.610, 0.559, 0.5080, // 23..25 (0.002"/gauge)
	0.4572, 0.4166, // (0.0016"/gauge)
	0.3759, 0.3454, // (0.0012"/gauge)
	// 30..38 (0.0008"/gauge)
	0.3150, 0.2946, 0.2743, 0.2540, 0.2337, 0.2134, 0.1930, 0.1727, 0.1524,
	// 39..48 (0.0004"/gauge)
	0.1321, 0.1219, 0.1118, 0.1016, 0.0914, 0.0813,
	0.0711, 0.0610, 0.0508, 0.0406,
	// 49 & 50 (0.0002"/guage)
	0.0305, 0.0254
};
	if ((n <= sizeof(SWG0_50)/sizeof(SWG0_50[0]))) { return SWG0_50[n] * 1E-3; }
	return(0); // NaN?
} // diamSWG

int printQT (const QTab *pQT, const SciFmtFuncPtr pSF, const char *fmt, const char *pre, const char *post, const char markN)
{
	int i, n=0;
	char ch[3];
	ch[1]= pQT->unit[0];
	ch[2]= 0;
	if (pre) { printf("%s", pre); }
	for (i= 0; i<pQT->ft.n; i++)
	{
		float v= pSF(ch, pQT->ft.p[i]);
		n+= printf(fmt, v, ch);
	}
	if (post) { printf("%s", post); }
	if (markN >= ' ') { for (i=0; i<n; i++) { printf("%c", markN); } }
	return(n);
} // printQT

void tabulateFunc (const QTab * pQT1, const QTab * pQT2, F1V1Desc *pF1V1D, F2V0Desc *pF2V0D, SciFmtFuncPtr pSF)
{
	char ch[8]={0}, fmt[12]=" %6.3G%s ";
	size_t i, j;
	int width= 6;

	if (sciFmtDummyF == pSF) { width= 8; }
	if (width <= 9) { fmt[2]= width+'0'; }

	printQT(pQT1, pSF, fmt, "\n\t\t   ", "\n\t\t   ", '_');

	snprintf(ch+1, 6, "%s", pQT2->unit);
	for (i=0; i < pQT2->ft.n; i++)
	{
		float f1= pQT2->ft.p[i];

		fmt[0]= '\n';
		fmt[8]= ' ';
		fmt[9]= 0;

      if (pF1V1D)
      {
         snprintf(ch+1, 6, "%s", pQT2->unit);
         printf(fmt, pSF(ch, f1), ch);
         f1= pF1V1D->pF1V1(f1, pF1V1D->p);
         snprintf(ch+1, 6, "%s", pF1V1D->unit);
         fmt[0]= ' ';
      }

      //fmt[8]=' ';
      fmt[9]='|';
		printf(fmt,  pSF(ch, f1), ch);
		fmt[0]=' '; fmt[8]= 0;

      snprintf(ch+1, 6, "%s", pF2V0D->unit);
		for (j=0; j < pQT1->ft.n; j++)
		{
			float f2= pQT1->ft.p[j];
			float r= pF2V0D->pF2V0(f1,f2);
			printf(fmt, pSF(ch, r), ch);
		}
	}
	printf("\n\n");
} // tabulateFunc

float circleDA (const float d) { return(M_PI * 0.25 * d * d); }

void defSolenoidParam (SolenoidParam *pSP, const float relPerm, const float xA, const float sT)
{
   if (0 != sT) { pSP->uAdS= relPerm * vacuum.permeability * xA / sT; } else { pSP->uAdS= 0; }
} // solenoidLT

float solenoid (float nT, const SolenoidParam *pSP) { return(nT * pSP->uAdS); }

// Series R-L  in parallel with c
float impedRLC (const float w, const float r, const float l, const float c)
{
	return( 1.0 / ( (w * c) + 1.0 / (r + (w * l)) ) );
} // impedRLC

float skinDepthWR (float wR, const MatProp *pMP)
{
	float wu= wR * pMP->permeability;
	if (0 != wu) { return sqrt( 2 * pMP->resistivity / wu ); }
	//else
	return(0);
} // skinDepthWR

float skinDepthHz (float fHz, const MatProp *pMP) { return skinDepthWR(2 * M_PI * fHz, pMP); }

float min (float a, float b) { return(a < b ? a : b); }
// 22.7mH (135T) 0.304nF -> 60.6KHz

float resFreqLC (const float l, const float c) { return( 1.0 / (2 * M_PI * sqrtf(l * c)) ); }

void test (void)
{
   const char s[]=" 12u5V \n";
   int n;
   double v;

   n= sciFmtScanF64(&v, s, strlen(s));
   printf("%G %d\n", v, n);
}

int main (int argc, char *argv[])
{
	SciFmtFuncPtr pSF= sciFmtSetF; // sciFmtDummyF
	QTab q1={0}, q2={0};
	char ch[8]={0}, fmt[12]=" %6.3G%s ";
	const float dS=9E-3;
	const float relPerm=600 * 3.0/8;
	float a, lW, dW, rW;
	float l, c, w, dw, f, df;
	int nT0, dT, swg=30;
   SolenoidParam solP;
   F1V1Desc f1v1d={(F1V1FuncPtr)solenoid,&solP,"H","Henry"};
   F2V0Desc f2v0d={resFreqLC,"Hz","Hertz"};

   procArgs(&gArgs, argc, (const char * const *)argv);
   test();
	dW= diamSWG(swg);
   printf("\nSWG%d : %G%cm\n", swg, pSF(ch+0,dW), ch[0]);
	nT0= 60;
   dT=   5;
	printf("%uT+%uT->l=%.3G%cm+%.3G%cm\n", nT0, dT, pSF(ch+0,nT0*dW), ch[0], pSF(ch+1,dT*dW), ch[1]);

   defSolenoidParam(&solP, relPerm, circleDA(dS), dW);

	defFTabLL(&(q1.ft), gArgs.cap.nStep, gArgs.cap.c.min, gArgs.cap.c.max); q1.unit= "F"; q1.label= "Farad";
	//defFTabSS(&(q2.ft), 20, sL, dL); q2.unit= "H"; q2.label= "Henry";
	defFTabSS(&(q2.ft), 20, nT0, dT); q2.unit= "T"; q2.label= "turns";

	c= q1.ft.p[14];
	l= q2.ft.p[19];
	w= 1 / sqrtf(l * c);
	f= w / (2 * M_PI);
	printf("l=%G,c=%G: w=%G, f=%G\n", l, c, w, f);

	tabulateFunc(&q1, &q2, &f1v1d, &f2v0d, pSF);

	lW= 0.018 / dW * M_PI * dS; // nT * circumference
	c= M_PI * dW;
	a= circleDA(dW);
	printf("skin depth (wire radius=%G%cm, area=%G%cmsq) :\n", pSF(ch+0, 0.5*dW), ch[0], pSF(ch+1, a), ch[1]);
	float fMHz[]={ 0.06, 0.198, 0.810 };
	for (int i=0; i<3; i++)
	{
      const float fHz= fMHz[i]*1E6;
		const float d= skinDepthHz(fHz, &Cu);
		const float dEA= d * c; // "magic" approximation?
		const float rma= 1.0 / min(a,dEA);
		rW= Cu.resistivity * lW * rma;
		printf("\t%3G%cHz -> %.3G%cm -> %.3G%cmsq -> %.3GOhms\n", pSF(ch+0,fHz), ch[0], pSF(ch+1, d), ch[1], pSF(ch+2, dEA), ch[2], rW);
	}
	//printf("lW=%G -> R=%G\n\n", lW, rW);

	//sprintf(ch+1, "R");
   ch[1]= 0;
	f= 1E6 * fMHz[0];
	df= 1E6 * 0.020; //(fMHz[2] - fMHz[0]) / 20;
   fmt[2]= '4'; fmt[8]= 0;
	l= solenoid(85,&solP);
   //q2.ft.p[5];
   c= q1.ft.p[14];
	printf("Impedance: (f0=%3G%cHz, df=%+3G%cHz, L=%.3G%cH, C=%.3G%cF)\n", pSF(ch+0,f), ch[0], pSF(ch+1,df), ch[1], pSF(ch+2,l), ch[2], pSF(ch+3,c), ch[3]);

   //snprintf(ch+1, sizeof(ch)-2, "Ohm");
   ch[1]= 0x0;
	w= M_PI * 2 * f;
	dw= M_PI * 2 * df;
	for (int i=0; i < 30; i++)
	{
		float z= impedRLC(w, rW, l, c); //q1.ft.p[0]);
		printf(fmt, pSF(ch, z), ch);
		w+= dw;
	}
	printf("\n\n");
	return 0;
} // main

