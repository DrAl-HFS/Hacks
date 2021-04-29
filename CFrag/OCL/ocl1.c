// ocl1.c - Minimal OpenCL device check
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr 2021

#include <stdio.h>
#include <string.h>
#include <CL/cl.h>

#define MAX_PF_ID    2
#define MAX_DEV_ID   8

int main (int argc, char *argv[])
{
   cl_platform_id idPfrm[MAX_PF_ID]={0,};
   cl_device_id idDev[MAX_DEV_ID]={0,};
   cl_uint nPfrm, nDev;
   cl_int r;

   printf("Build target : OpenCl V%u\n", CL_TARGET_OPENCL_VERSION);

   r= clGetPlatformIDs(MAX_PF_ID, idPfrm, &nPfrm);
   printf("DBG: clGetPlatformIDs() - %d [%u]\n", r, nPfrm);

   for(int i = 0; i < nPfrm; i++)
   {
      printf("DBG: id=0x%X\n", idPfrm[i]);
      r= clGetDeviceIDs(idPfrm[i], CL_DEVICE_TYPE_ALL, MAX_DEV_ID, idDev, &nDev);
      printf("DBG: clGetDeviceIDs( *ALL ) - %d [%u]\n", r, nDev);
      if (r < 0)
      {
         r= clGetDeviceIDs(idPfrm[i], CL_DEVICE_TYPE_CPU, MAX_DEV_ID, idDev, &nDev);
         printf("DBG: clGetDeviceIDs( *CPU ) - %d [%u]\n", r, nDev);

         r= clGetDeviceIDs(idPfrm[i], CL_DEVICE_TYPE_GPU, MAX_DEV_ID, idDev, &nDev);
         printf("DBG: clGetDeviceIDs( *GPU ) - %d [%u]\n", r, nDev);
      }
   }
   return((nDev > 0)-1);
} // main
