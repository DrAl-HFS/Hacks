// ocl1.c - Minimal OpenCL device check
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr - May 2021

// RaspiOS64 (POCL) setup:
// > sudo apt install opencl-* clinfo
// Installs clang-6.0 without generic <clang> link so do:
// > sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang
// NB: POCL and OpenCL in general appear to be non-functional
// under RaspiOS32. (YMMV?)

#include <stdio.h>
#include <string.h>
#include <CL/cl.h>

#define MAX_PF_ID    2
#define MAX_DEV_ID   8

const char *gAF[2]={"[%u]\n", "[%X]\n"};
void printAlt (const char *af[], const int i, const size_t x) { printf(af[i], x); }

int main (int argc, char *argv[])
{
   cl_platform_id idPfrm[MAX_PF_ID]={0,};
   cl_device_id idDev[MAX_DEV_ID]={0,};
   cl_uint nPfrm, nDev;
   char buf[64];
   const char *s;
   size_t b=0;
   cl_int r;

   printf("Build target : OpenCl V%u\n", CL_TARGET_OPENCL_VERSION);

   r= clGetPlatformIDs(MAX_PF_ID, idPfrm, &nPfrm);
   printf("DBG: clGetPlatformIDs() - %d [%u]\n", r, nPfrm);

   for (int i = 0; i < nPfrm; i++)
   {
      r= clGetPlatformInfo(idPfrm[i], CL_PLATFORM_NAME, sizeof(buf)-1, buf, &b);
      if ((r < 0) || (b <= 0)) { s= "?"; } else { s= buf; }
      printf("DBG: platform id= 0x%X -> %s\n", (cl_uint)idPfrm[i], s);

      nDev= 0;
      r= clGetDeviceIDs(idPfrm[i], CL_DEVICE_TYPE_ALL, MAX_DEV_ID, idDev, &nDev);
      printf("DBG: clGetDeviceIDs( *ALL ) - %d ", r); printAlt(gAF, nDev >= (1<<20), nDev);
      for (int i = 0; i < nDev; i++)
      {
         s= buf;
         r= clGetDeviceInfo(idDev[i], CL_DEVICE_NAME, sizeof(buf)-1, buf, &b);
         if ((r < 0) || (b <= 0)) { s= "?"; } else { s= buf; }
         printf("DBG: device id= 0x%X -> [%u] %s\n", (cl_uint)idDev[i], b, s);

         b= 0;
         r= clGetDeviceInfo(idDev[i], CL_DRIVER_VERSION, sizeof(buf)-1, buf, &b);
         if ((r < 0) || (b <= 0)) { s= "?"; } else { s= buf; }
         printf("DBG: version [%u] %s\n", b, s);
     }
   }
   return((nDev > 0)-1);
} // main
