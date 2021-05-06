// ocl1.cpp - Minimal OpenCL device test, migrated to C++.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr - May 2021

// For Debian-flavoured Linux on x86, Jetson-Nano, RPi3,4 (64bit OS required).
// POCL (CPU SIMD only acceleration) setup:
// > sudo apt install *opencl-* clinfo
// Note that POCL relies on the "clang" compiler driver i.e. "llvm" compiler tools.
// If clang-6.0 (or whatever) is installed without a generic <clang> link, then it
// may be necessary to create one e.g:
// > sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang
// or just give the more specific name in the Makefile...

//#include <stdio.h>
#include <new>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <CL/cl.h>


/***/

#define MAX_PF_ID    2
#define MAX_DEV_ID   4


typedef float Scalar;

// Minimal information required to use a device
class CSimpleOCL
{
public:
   cl_context        ctx;
   cl_command_queue  q;
   
   CSimpleOCL (cl_device_id id=0) : ctx{0},q{0} { if (0 != id) { create(id); } }
   
   ~CSimpleOCL () { release(); }

   bool create (cl_device_id id)
   {
      cl_int r;
      ctx= clCreateContext(NULL, 1, &id, NULL, NULL, &r);
      if (r >= 0)
      { 
         q= clCreateCommandQueueWithProperties(ctx, id, NULL, &r); // OpenCL 2.0+
         //q= clCreateCommandQueue(ctx, id, 0, &r);
      }
      return(r >= 0);
   } // create

   bool release (void)
   {
      cl_int r= clReleaseCommandQueue(q);
      q= 0;
      r= clReleaseContext(ctx);
      ctx= 0;
      return(r >= 0);
   }
}; // CSimpleOCL

// Build a simple kernel
class CBuildOCL : public CSimpleOCL
{
public:
   cl_program idProg;
   cl_kernel  idKern;

   CBuildOCL (cl_device_id id=0) : CSimpleOCL(id) { ; }
   ~CBuildOCL () { release(true); }

   bool defaultBuild (const char src[], const char name[])
   {
      cl_int r;
      idProg= clCreateProgramWithSource(ctx, 1, &src, NULL, &r);
      //std::cout << "clCreateProgramWithSource() - r=%d" << r);
      if (r >= 0)
      {  // simple build for default device
         r= clBuildProgram(idProg, 0, NULL, NULL, NULL, NULL);
         //std::cout << "clBuildProgram() - r=%d" << r);
         if (r >= 0)
         {  // Create the compute kernel in the program we wish to run
            idKern= clCreateKernel(idProg, name, &r);
            //std::cout << "clCreateKernel() - r=%d" << r);
         }
      }
      return(r >= 0);
   } // defaultBuild

   bool release (bool all=true) // override
   {
      cl_int r= clReleaseProgram(idProg);
      idProg= 0;
      r= clReleaseKernel(idKern);
      idKern= 0;
      if (all) { return((r>=0) && CSimpleOCL::release()); }
      //else
      return(r >= 0);
   } // release
}; // CBuildOCL

/***/

// Finally, the actual application: vector addition

const char vecAddSrc[]=
"__kernel void vecAdd(  __global float *pR, __global float *pA, __global float *pB, const size_t n)\n" \
"{ int id= get_global_id(0); if (id < n) { pR[id]= pA[id] + pB[id]; } }";

struct HostArgs
{
   Scalar *pR, *pA, *pB;   // host memory buffers
   size_t n; // elements in each buffer (on both host and device)

   HostArgs (void) : pR{NULL}, pA{NULL}, pB{NULL}, n{0} { ; }
   
   bool allocate (size_t nElem)
   {
      if ((NULL == pR) && (NULL == pA) && (NULL == pB))
      {
         n= nElem;
         pR= new Scalar[n]; pA= new Scalar[n]; pB= new Scalar[n];
         return(pR && pA && pB);
      }
      //else
      return(false);
   }
   bool release (void)
   {
      delete [] pR; delete [] pA; delete [] pB;
      pR= pA= pB= NULL;
      return(true);
   }
}; // HostArgs

struct DeviceArgs
{
   cl_mem hR, hA, hB; // device buffer "handle" identifiers
   size_t bytes;  // size of each buffer (on both host and device)
   
   DeviceArgs (void) : hR{0}, hA{0}, hB{0}, bytes{0} { ; }
   bool allocate (size_t nElem, cl_context ctx)
   {
      cl_int r[3];
      bytes= sizeof(Scalar) * nElem;
      hR= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, r+0);
      hA= clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, NULL, r+1);
      hB= clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, NULL, r+2);
      return((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0));
   }
   bool release (void)
   {
      cl_int r[3];
      r[0]= clReleaseMemObject(hR);
      r[1]= clReleaseMemObject(hA);
      r[2]= clReleaseMemObject(hB);
      hR= hA= hB= 0;
      return((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0));
   }
}; // DeviceArgs


/***/

void initData (const HostArgs& h)
{
   for (size_t i= 0; i < h.n; i++)
   {
      double theta= i * M_PI / (1<<8);
      h.pA[i] = sinf(theta)*sinf(theta);
      h.pB[i] = cos(theta)*cos(theta);
      h.pR[i]= -1;
   }
} // initData

Scalar sum (const Scalar v[], const size_t n)
{
   Scalar s= v[0]; // hacky assumption n>=1
   for (size_t i=1; i<n; i++) { s+= v[i]; }
   return(s);
} // sum


/***/


class CVecAddOCL : public CBuildOCL
{
protected:
   // return number of work groups for local & problem size
   size_t nwg (size_t l, size_t n) { return((n + l - 1) / l); }
   
   HostArgs    host;
   DeviceArgs  device;

public:
   bool createArgs (size_t nElem)
   { 
      if (nElem > 0) { return( host.allocate(nElem) && device.allocate(nElem, CSimpleOCL::ctx) ); }
      //else
      return(false);
   }

   CVecAddOCL (size_t nElem=0) { createArgs(nElem); }
   ~CVecAddOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws)
   {
      size_t gws= lws * nwg(lws, host.n);
      cl_event evt[4];
      cl_int r, wr[2], ar[4];

      //std::cout << "%zub, ws: %zu %zu" << bytes, lws, gws);

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hR), &(device.hR));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(device.hA), &(device.hA));
      ar[2]= clSetKernelArg(CBuildOCL::idKern, 2, sizeof(device.hB), &(device.hB));
      ar[3]= clSetKernelArg(CBuildOCL::idKern, 3, sizeof(host.n), &(host.n));
      //std::cout << "ar: %d %d %d %d" << ar[0], ar[1], ar[2], ar[3]);

      // Copy (sync.) input buffers
      wr[0]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hA, CL_TRUE, 0, device.bytes, host.pA, 0, NULL, evt+0);
      wr[1]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hB, CL_TRUE, 0, device.bytes, host.pB, 0, NULL, evt+1);
      //std::cout << "wr: %d %d" << wr[0], wr[1]);

      // Submit kernel job
      r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 1, NULL, &gws, &lws, 0, NULL, evt+2);

      if (r >= 0)
      {
         std::cout << "job enqueued" << std::endl;
         clFinish(CSimpleOCL::q); // Global sync

         // Read the results (sync.) from the device
         r= clEnqueueReadBuffer(CSimpleOCL::q, device.hR, CL_TRUE, 0, device.bytes, host.pR, 0, NULL, evt+3);
      }
      else { std::cout << "enqueue r=" << r << std::endl; }
      return(r >= 0);
   } // execute

   void initHostData (void) { initData(host); }
   
   Scalar sumR (void) { return sum(host.pR, host.n); }
   
   size_t getN (void) { return(host.n); }
   
   bool release (bool all=true)
   { 
      bool r= host.release() && device.release();
      if (all) { r&= CBuildOCL::release(all); }
      return(r);
   }
}; // CVecAddOCL


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

   std::cout << "Build target : OpenCl V" << CL_TARGET_OPENCL_VERSION << std::endl;
   if (clGetPlatformIDs(MAX_PF_ID, idPfrm, &nPfrm) >= 0)
   {
      std::cout << nPfrm << "platform(s):" << std::endl;

      for (int iP= 0; iP < nPfrm; iP++)
      {
         r= clGetPlatformInfo(idPfrm[iP], CL_PLATFORM_NAME, sizeof(buf)-1, buf, &b);
         if ((r < 0) || (b <= 0)) { s[0]= "?"; } else { s[0]= buf; }
         std::cout << "platform id= " << idPfrm[iP] << "->" << s[0] << ":" << std::endl;

         int remD= maxD-nDev;
         if (remD < 0) { remD= 0; }
         if (clGetDeviceIDs(idPfrm[iP], CL_DEVICE_TYPE_ALL, remD, idDev+nDev, &n) >= 0)
         {
            std::cout << n << " device(s):" << std::endl;
            for (int iD= nDev; iD < nDev+n; iD++)
            {
               const cl_device_info ditok[]={ CL_DEVICE_NAME, CL_DEVICE_VENDOR, CL_DEVICE_VERSION, CL_DRIVER_VERSION };
               getDevStr(s+1, buf, sizeof(buf)-1, idDev[iD], ditok, 4);
               std::cout << "\t" << s[1] << "(" << s[2] << ")" << s[3] << "(Driver V" << s[4] << ")" << std::endl;
            }
            nDev+= n;
         }
      }
   }
   return(nDev);
} // queryDev


/***/

int main (int argc, char *argv[])
{
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDev(idDev, MAX_DEV_ID);
   int r=-1;

   if (nDev > 0)
   {
      CVecAddOCL va;
      if (va.create(idDev[0]) && va.createArgs(1<<20))
      {
         std::cout << "context created" << std::endl;
         if (va.defaultBuild(vecAddSrc,"vecAdd"))
         {
            std::cout << "build OK" << std::endl;
            va.initHostData();

            if (va.execute(32))
            { 
               Scalar s= va.sumR();
               size_t e= va.getN();
               Scalar re= 2 * abs(e-s) / (e + s);
               std::cout << "result: sum=" << s << " expected=" << e << std::endl;
               std::cout << "relative error=" << re << std::endl;
               if (re <= 1E-6) { r= 0; }
            }
         }
      }
   }
   return(r);
} // main
