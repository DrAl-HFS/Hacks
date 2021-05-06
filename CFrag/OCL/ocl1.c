// ocl1.c - Minimal OpenCL device test
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr - May 2021

// For Debian-flavoured Linux on x86, Jetson-Nano, RPi3,4 (64bit OS required).
// POCL (CPU SIMD only acceleration) setup:
// > sudo apt install *opencl-* clinfo
// NB: may install clang-6.0 without generic <clang> link so do:
// > sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <CL/cl.h>


/***/

#define MAX_PF_ID    2
#define MAX_DEV_ID   4


typedef float Scalar;

// Minimal information required to use a device
typedef struct
{
   cl_context        ctx;
   cl_command_queue  q;
} SimpleOCL;

typedef struct
{
   cl_program idProg;
   cl_kernel  idKern;
} BuildOCL;

typedef struct
{
   Scalar *pR, *pA, *pB;   // host memory buffers
   cl_mem dhR, dhA, dhB;   // device buffer "handle" ids
   size_t n; // elements in each buffer
} ArgsOCL;

typedef struct
{
   size_t local, global;
} WorkPattern;


/***/

const char src[]=
"__kernel void vecAdd(  __global float *pR, __global float *pA, __global float *pB, const size_t n)\n" \
"{ int id= get_global_id(0); if (id < n) { pR[id]= pA[id] + pB[id]; } }";

/***/

// Generate table of strings (c-string i.e. nul-terminated ASCII) from tokens
size_t getDevStr
(
   const char *s[],     // string table (result)
   char buf[],           // buffer to hold strings
   const int maxBuf,    // max size of buffer
   const cl_device_id id,  // device to query
   const cl_device_info tok[],   // token array
   const int n    // number of tokens (and strings)
)
{
   int nB=0;
   for (int i=0; i<n; i++)
   {
      int rem= maxBuf-nB;
      if (rem > 0)
      {
         size_t b= 0;
         if ((clGetDeviceInfo(id, tok[i], rem, buf+nB, &b) < 0) || (b <= 1))
         {
            buf[nB++]= '?'; buf[nB++]= 0x0; // generate placeholder for error/incomplete
         }
         else
         {
            s[i]= buf+nB;
            nB+= b;
         }
      } else { s[i]= NULL; }
   }
   return(nB);
} // getDevStr

int queryDev (cl_device_id idDev[], int maxD)
{
   char buf[1<<8];
   cl_platform_id idPfrm[MAX_PF_ID]={0,};
   cl_uint nPfrm, n;
   int nDev=0;
   const char *s[5];
   size_t b=0;
   cl_int r;

   printf("Build target : OpenCl V%u\n", CL_TARGET_OPENCL_VERSION);
   if (clGetPlatformIDs(MAX_PF_ID, idPfrm, &nPfrm) >= 0)
   {
      printf("%u platform(s):\n", nPfrm);

      for (int iP= 0; iP < nPfrm; iP++)
      {
         r= clGetPlatformInfo(idPfrm[iP], CL_PLATFORM_NAME, sizeof(buf)-1, buf, &b);
         if ((r < 0) || (b <= 0)) { s[0]= "?"; } else { s[0]= buf; }
         printf("platform id= 0x%X -> %s:\n", (cl_uint)idPfrm[iP], s[0]);

         int remD= maxD-nDev;
         if (remD < 0) { remD= 0; }
         if (clGetDeviceIDs(idPfrm[iP], CL_DEVICE_TYPE_ALL, remD, idDev+nDev, &n) >= 0)
         {
            printf("%u device(s):\n", n);
            for (int iD= nDev; iD < nDev+n; iD++)
            {
               const cl_device_info ditok[]={ CL_DEVICE_NAME, CL_DEVICE_VENDOR, CL_DEVICE_VERSION, CL_DRIVER_VERSION };
               getDevStr(s+1, buf, sizeof(buf)-1, idDev[iD], ditok, 4);
               printf("\t%s (%s) %s (Driver V%s)\n", s[1], s[2], s[3], s[4]);
            }
            nDev+= n;
         }
      }
   }
   return(nDev);
} // queryDev

int createSimple (SimpleOCL *pS, cl_device_id id)
{
   cl_int r;
   pS->ctx= clCreateContext(NULL, 1, &id, NULL, NULL, &r);
   if (r >= 0)
   {
      pS->q= clCreateCommandQueue(pS->ctx, id, 0, &r);
   }
   return(r);
} // createSimple

int createArgs (ArgsOCL *pA, const SimpleOCL *pS, size_t n)
{
   size_t bytes= n * sizeof(*(pA->pR));
   cl_int d[3]={-1,};

   pA->n= n;

   pA->pR= malloc(bytes);
   pA->pA= malloc(bytes);
   pA->pB= malloc(bytes);

   if (pA->pR && pA->pA && pA->pB)
   {
      printf("host buffers allocated\n");
      pA->dhR= clCreateBuffer(pS->ctx, CL_MEM_WRITE_ONLY, bytes, NULL, d+0);//pA->pR
      pA->dhA= clCreateBuffer(pS->ctx, CL_MEM_READ_ONLY, bytes, NULL, d+1);//pA->pA
      pA->dhB= clCreateBuffer(pS->ctx, CL_MEM_READ_ONLY, bytes, NULL, d+2);//pA->pB
      //printf("dh: %p %p %p\n", pA->dhR, pA->dhA, pA->dhB);
      //printf("d: %d %d %d\n", d[0], d[1], d[2]);
   }
   return(d[0]);
} // createArgs

void initData (const ArgsOCL *pA)
{
   for (size_t i= 0; i < pA->n; i++)
   {
      double theta= i * M_PI / (1<<8);
      pA->pA[i] = sinf(theta)*sinf(theta);
      pA->pB[i] = cos(theta)*cos(theta);
      pA->pR[i]= -1;
   }
} // initData

int buildDefault (BuildOCL *pB, const SimpleOCL *pS, const char src[])
{
   cl_int r;
   pB->idProg= clCreateProgramWithSource(pS->ctx, 1, &src, NULL, &r);
   //printf("clCreateProgramWithSource() - r=%d\n", r);
   if (r >= 0)
   {
      r= clBuildProgram(pB->idProg, 0, NULL, NULL, NULL, NULL); // simple build for default device
      //printf("clBuildProgram() - r=%d\n", r);
      if (r >= 0)
      {  // Create the compute kernel in the program we wish to run
         pB->idKern= clCreateKernel(pB->idProg, "vecAdd", &r);
         //printf("clCreateKernel() - r=%d\n", r);
      }
   }
   return(r);
} // buildDefault

size_t nwg (size_t l, size_t n) { return((n + l - 1) / l); }

int execute (BuildOCL *pB, const ArgsOCL *pA, const SimpleOCL *pS, size_t lws)
{
   cl_event evt[4];
   size_t bytes= pA->n * sizeof(*(pA->pA));
   size_t gws= lws * nwg(lws, pA->n);
   cl_int r, w[2], a[4];

   //printf("%zub, ws: %zu %zu\n", bytes, lws, gws);

   // Set args on device
   a[0]= clSetKernelArg(pB->idKern, 0, sizeof(pA->dhR), &(pA->dhR));
   a[1]= clSetKernelArg(pB->idKern, 1, sizeof(pA->dhA), &(pA->dhA));
   a[2]= clSetKernelArg(pB->idKern, 2, sizeof(pA->dhB), &(pA->dhB));
   a[3]= clSetKernelArg(pB->idKern, 3, sizeof(pA->n), &(pA->n));
   //printf("a: %d %d %d %d\n", a[0], a[1], a[2], a[3]);

   // Copy (sync.) input buffers
   w[0]= clEnqueueWriteBuffer(pS->q, pA->dhA, CL_TRUE, 0, bytes, pA->pA, 0, NULL, evt+0);
   w[1]= clEnqueueWriteBuffer(pS->q, pA->dhB, CL_TRUE, 0, bytes, pA->pB, 0, NULL, evt+1);
   //printf("w: %d %d\n", w[0], w[1]);

   // Submit kernel job
   r= clEnqueueNDRangeKernel(pS->q, pB->idKern, 1, NULL, &gws, &lws, 0, NULL, evt+2);

   if (r >= 0)
   {
      printf("job enqueued\n");
      clFinish(pS->q); // Global sync

      // Read the results from the device
      r= clEnqueueReadBuffer(pS->q, pA->dhR, CL_TRUE, 0, bytes, pA->pR, 0, NULL, evt+3);
   }
   else { printf("enqueue r=%d\n", r); }
   return(r);
} // execute

int releaseBuild (BuildOCL *pB)
{
    cl_int r= clReleaseProgram(pB->idProg);
    r= clReleaseKernel(pB->idKern);
    return(r);
} // releaseBuild

int releaseArgs (ArgsOCL *pA)
{
   cl_int r;
   free(pA->pR);
   free(pA->pA);
   free(pA->pB);
   pA->pR= pA->pA= pA->pB= NULL;
   r= clReleaseMemObject(pA->dhR);
   r= clReleaseMemObject(pA->dhA);
   r= clReleaseMemObject(pA->dhB);
   return(r);
} // releaseArgs

int releaseSimple (SimpleOCL *pS)
{
   cl_int r= clReleaseCommandQueue(pS->q);
   if (r >= 0) { pS->q= 0; }
   r= clReleaseContext(pS->ctx);
   if (r>= 0) { pS->ctx= 0; }
   return(r);
} // releaseSimple

Scalar sum (const Scalar v[], const size_t n)
{
   Scalar s= v[0]; // hacky assumption n>=1
   for (size_t i=1; i<n; i++) { s+= v[i]; }
   return(s);
} // sum

int main (int argc, char *argv[])
{
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDev(idDev, MAX_DEV_ID);
   int r=-1;

   if (nDev > 0)
   {
      SimpleOCL simple;
      ArgsOCL args;
      BuildOCL build;
      if ((createSimple(&simple, idDev[0]) >= 0) && (createArgs(&args, &simple, 1<<20) >= 0))
      {
         printf("context created\n");
         if (buildDefault(&build, &simple, src) >= 0)
         {
            printf("build OK\n");
            initData(&args);

            r= execute(&build, &args, &simple, 32);

            if (r >= 0) { printf("result sum=%G\n", sum(args.pR, args.n)); }

            releaseBuild(&build);
         }
         releaseArgs(&args);
         releaseSimple(&simple);
      }
   }
   return(r);
} // main
