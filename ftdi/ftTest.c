// ftTest.h - FTDI device hacking test.
// https://github.com/DrAl-HFS/  ???Hacks.git
// Licence: GPL V3
// (c) Project Contributors August-Sept 2020

#include <libftdi1/ftdi.h>
#include <stdio.h>
#include <unistd.h>

// Useful:-
// https://www.linux.com/training-tutorials/linux-kernel-module-management-101


// USB device id values
#define ID_VNDR_FTDI	0x0403
#define ID_PROD_FT2232H	0x6010
#define ID_PROD_FT232	0x6001

#define SEC_US 1000000 // microseconds per second

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int    U32;
typedef struct ftdi_context FTCtx;

void ftDumpLibInfo (void)
{
   struct ftdi_version_info v={0,};
   v= ftdi_get_library_version();
   printf("libftdi V%d.%d.%d\n", v.major, v.minor, v.micro);
   //else { printf("ERROR: ..%s() - r=%d\n", "get_library_version", r); }
} // ftDumpLibInfo

void ftDumpDevInfo (FTCtx *pFC)
{
   int r=-1;
   //char mfr[64], desc[64], ser[64]; 
   //r= ftdi_usb_get_strings(pFC, usb_dev???, mfr, sizeof(mfr), desc, sizeof(desc), ser, sizeof(ser));
   //r= ftdi_eeprom_get_strings(pFC, mfr, sizeof(mfr)-1, desc, sizeof(desc)-1, ser, sizeof(ser)-1);
   //if (r >= 0) { printf("M,D,S:-\n%s\n%s\n%s\n",mfr,desc,ser); }

   //U32 id; r= ftdi_read_chipid(&gFC, &id);
   //if (r >= 0) { printf("ID: %X\n", id); }
   //else { printf("ERROR: ..%s() - r=%d\n", "read_chipid", r); }

   printf("DevTyp %d\tbaud=%d\n", pFC->type, pFC->baudrate);
   printf("IF/Idx: %d, %d\n", pFC->interface, pFC->index);
   printf("EEPROM: %p\n", pFC->eeprom);
   
   //ftdi_set_eeprom_value(pFC, 
   //for (int i= CBUS_FUNCTION_0; i<=CBUS_FUNCTION_9; i++)
   for (int i= VENDOR_ID; i<=PRODUCT_ID; i++)
   {
      int val=0xFFF0 ^ i;
      //r= ftdi_read_eeprom_location(pFC, i, &val);
      r= ftdi_get_eeprom_value(pFC, i, &val);
      if (r >= 0)
      { 
         printf("%04x : %04x\n",i, val);
      }
   }
   //r= ftdi_eeprom_decode(pFC, 1);
} // ftDumpDevInfo

FTCtx *ftInitUSB (U16 devid, const enum ftdi_interface ifid)
{
   FTCtx *pFC=NULL;
   int r; 
//static FTCtx gFC={0,};
// if (ftdi_init(&gFC) >= 0) { pFC= &gFC; } else { printf("ERROR: ..%s() - r=%d\n", "init", r); }
   pFC= ftdi_new();
   if (pFC)
   {
      ftDumpLibInfo();

      if (ifid > INTERFACE_ANY)
      {
         r= ftdi_set_interface(pFC, ifid);
         if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_interface", ifid, r); }
      }
      
      r= ftdi_usb_open(pFC, ID_VNDR_FTDI, devid);

      if (r < 0) { printf("ERROR: ..%s() - r=%d\n", "usb_open", r); }
      else
      {
         ftDumpDevInfo(pFC);
         return(pFC);
      }
   }
   if (pFC) { ftdi_deinit(pFC); } else { printf("ERROR: ..%s() - %p\n", "new", pFC); }
   return(NULL);
} // ftInitUSB

int ftSetModeIF (FTCtx *pFC, const enum ftdi_interface ifid, const U8 m, const enum ftdi_mpsse_mode bmid)
{
   int r=-1;
   if (pFC)
   {
      if (ifid > INTERFACE_ANY)
      {
         r= ftdi_set_interface(pFC, ifid);
         if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_interface", ifid, r); }
      }
      if (0 != bmid) { ftdi_set_bitmode(pFC, 0, 0); } // reset
      r= ftdi_set_bitmode(pFC, m, bmid);
      if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_bitmode", bmid, r); }
   }
   return(r >= 0);
} // ftSetModeIF

void flashBang (FTCtx *pFC, U8 state, const U8 flash, const int t, const int n)
{
   for (int i=0; i<n; i++)
   {
      ftdi_write_data(pFC, &state, sizeof(state));
      usleep(t);
      state^= flash;
   }
   if (0 == state) { ftdi_write_data(pFC, &state, sizeof(state)); }
} // flashBang

void flashBang2 (FTCtx *pFCA, FTCtx *pFCB, U8 state, const U8 flash, const int t, const int n)
{
   for (int i=0; i<n; i++)
   {
      ftdi_write_data(pFCA, &state, sizeof(state));
      state^= flash;
      ftdi_write_data(pFCB, &state, sizeof(state));
      usleep(t);
   }
   state= 0;
   ftdi_write_data(pFCA, &state, sizeof(state));
   ftdi_write_data(pFCB, &state, sizeof(state));
} // flashBang

void ftCleanup (FTCtx *pFC)
{
   if (pFC)
   {
      //ftSetModeIF(pFC, 0, 0, BITMODE_RESET);
      int r= ftdi_usb_reset(pFC);
      if (r < 0) { printf("ERROR: ..%s() -> %d\n", "usb_reset", r); }
      r= ftdi_usb_close(pFC);
      if (r < 0) { printf("ERROR: ..%s() -> %d\n", "usb_close", r); }
      ftdi_deinit(pFC);
   }
} // ftCleanup

// BITMODE_MPSSE
// https://gist.github.com/bjornvaktaren/d2461738ec44e3ad8b3bae4ce69445b4

int main (int argc, char *argv[])
{
   U16 devid= ID_PROD_FT232; //ID_PROD_FT2232H;
   FTCtx *pFCA, *pFCB= NULL;
   
   pFCA= ftInitUSB(devid, INTERFACE_A);
   if (ID_PROD_FT2232H == devid) { pFCB= ftInitUSB(devid, INTERFACE_B); }

   if (ftSetModeIF(pFCA, 0, 0xF0, BITMODE_BITBANG))
   {
      if (ftSetModeIF(pFCB, 0, 0xF0, BITMODE_BITBANG))
      {
         printf("flashBang2(A,B) ...\n");
         flashBang2(pFCA, pFCB, 0x0, 0xF0, SEC_US/25, 100);
      }
      else
      {
         printf("flashBang(A) ...\n");
         flashBang(pFCA, 0x0, 0xF0, SEC_US/25, 100);
      }
      printf("... usleep() ...\n");
      usleep(100000);
   }
   ftCleanup(pFCA);
   ftCleanup(pFCB);
   return(0);
} // main
