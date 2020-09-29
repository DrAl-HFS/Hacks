// ftTest.h - FTDI device hacking test.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: GPL V3
// (c) Project Contributors August-Sept 2020

#include <libftdi1/ftdi.h>
#include <stdio.h>
#include <unistd.h>

// USB device id values
#define ID_VNDR_FTDI    0x0403
#define ID_PROD_FT4232  0x6014
#define ID_PROD_FT2232	0x6010
#define ID_PROD_FT232	0x6001

#define MICRO_TICKS 1000000 // microseconds per second

#ifndef PLATFORM_H
typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int    U32;
typedef int Bool; // Boolean
#endif

#ifndef REPORT_H
#define LOG printf
#endif

typedef struct ftdi_context FTCtx;

static FTCtx *gpFC=NULL;

Bool ftNoError (const char id[], int r, const void *pArg)
{
   if (r < 0)
   {
      const char *pES=NULL;
      char sArg[32];
      if (pArg) { snprintf(sArg, sizeof(sArg)-1, "%d", ((int*)pArg)[0]); } else { sArg[0]= 0; }
      if (gpFC) { pES= ftdi_get_error_string(gpFC); }
      if (NULL == pES) { pES= ""; }
      printf("ERROR: ..%s(%s) -> %d %s\n", id, sArg, r, pES);
      return(0);
   }
   //else
   return(1);
} // ftNoError

int ftDevPortCount (const U16 devid)
{
   switch(devid)
   {
      case ID_PROD_FT2232 : return(2);
      case ID_PROD_FT4232 : return(4);
   }
   return(1);
} // ftDevPortCount

void ftDumpLibInfo (void)
{
   struct ftdi_version_info v={0,};
   v= ftdi_get_library_version();
   LOG("libftdi V%d.%d.%d\n", v.major, v.minor, v.micro);
} // ftDumpLibInfo

void ftDumpDevInfo (FTCtx *pFC)
{
   //int r=-1;
   //char mfr[64], desc[64], ser[64];
   //r= ftdi_usb_get_strings(pFC, usb_dev???, mfr, sizeof(mfr), desc, sizeof(desc), ser, sizeof(ser));
   //r= ftdi_eeprom_get_strings(pFC, mfr, sizeof(mfr)-1, desc, sizeof(desc)-1, ser, sizeof(ser)-1);
   //if (r >= 0) { LOG("M,D,S:-\n%s\n%s\n%s\n",mfr,desc,ser); }

   //U32 id; r= ftdi_read_chipid(&gFC, &id);
   //if (r >= 0) { LOG("ID: %X\n", id); }
   //else { LOG("ERROR: ..%s() - r=%d\n", "read_chipid", r); }
   LOG("DevTyp %d\trate= %d bd (Clk %d Hz)\n", pFC->type, pFC->baudrate, 16 * pFC->baudrate);
   LOG("IF/Idx: %d, %d\n", pFC->interface, pFC->index);
/* Nothing useful happening here...
   r= ftdi_read_eeprom(pFC);
   printf("EEPROM: %d -> %p:\n", r, pFC->eeprom);
   for (int fid= CBUS_FUNCTION_0; fid<=CBUS_FUNCTION_9; fid++)
   {
      int val= 0xA5FFFF00 ^ fid;
      r= ftdi_get_eeprom_value(pFC, fid, &val);
      LOG("[%d] - %d - 0x%X\n", fid, r, val);
   }
   LOG("\n--\n");

   for (int i= VENDOR_ID; i<=PRODUCT_ID; i++)
   {
      int val=0xA5FFFF00 ^ i;
      //r= ftdi_read_eeprom_location(pFC, i, &val);
      r= ftdi_get_eeprom_value(pFC, i, &val);
      LOG("r%d [%d] : %04x\n", r, i, val);
   }
*/
} // ftDumpDevInfo

FTCtx *ftInitUSB (const U16 devid, const enum ftdi_interface ifid, const int br, const U8 flags)
{
   FTCtx *pFC= ftdi_new();
   if (pFC)
   {
      int r;

      if (NULL == gpFC) { gpFC= pFC; }
      if (flags & (1<<7)) { ftDumpLibInfo(); }

      if (ifid > INTERFACE_ANY)
      {  // Multiple interfaces on a device need setup before USB binding (for endpoint definition ?)
         r= ftdi_set_interface(pFC, ifid);
         ftNoError("set_interface", r, &ifid);
      }

      r= ftdi_usb_open(pFC, ID_VNDR_FTDI, devid);

      if (ftNoError("usb_open", r, &devid))
      {
         if (br > 0)
         {
            r= ftdi_set_baudrate(pFC, br);
            ftNoError("set_baudrate", r, &br);
         }
         if (flags & (1<<6)) { ftDumpDevInfo(pFC); }
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
      {  // Seems not to work when multiple interfaces in use...
         r= ftdi_set_interface(pFC, ifid);
         ftNoError("set_interface", r, &ifid);
      }
      if (0 != bmid) { r= ftdi_set_bitmode(pFC, 0, 0); } // reset
      r= ftdi_set_bitmode(pFC, m, bmid);
      ftNoError("set_bitmode", r, &bmid);
   }
   return(r >= 0);
} // ftSetModeIF

FTCtx *ftCleanup (FTCtx *pFC, const U8 flags)
{
   int r= -1;
   if (pFC)
   {
      if (0 == (flags & 0x1)) { r= ftSetModeIF(pFC, 0, 0, BITMODE_RESET); }
      if (0 == (flags & 0x2))
      {
         r= ftdi_disable_bitbang(pFC);	// Waste of time? Neither causes SIO reload...
         ftNoError("disable_bitbang", r, NULL);
      }
      if (0 == (flags & 0x4))
      {
         r= ftdi_usb_reset(pFC);
         ftNoError("usb_reset", r, NULL);
      }
      if (0 == (flags & 0x8))
      {
         r= ftdi_usb_close(pFC);
         ftNoError("usb_close", r, NULL);
      }
      ftdi_free(pFC);
      if (gpFC == pFC) { gpFC= NULL; }
   }
   return(NULL);
} // ftCleanup

void flashBang (FTCtx *pFC, const U8 flash, const int usIvl, const int nIvl)
{
   int nS= 256;
   U8 state[256];
   //printf("flashBang() - sizeof(state)=%d\n", sizeof(state));
   for (int j= 0; j<nS; j++) { state[j]= nS-j; } //0xFF; }
   for (int i=0; i<nIvl; i++)
   {
      //state[0]= i;
      //state[0xFF]= state[0] ^ 0xFF;
      ftdi_write_data(pFC, state, nS);
      usleep(usIvl);
   }
   state[0]= 0x00; //FF;
   ftdi_write_data(pFC, state, 1);
} // flashBang

void flashBang2 (FTCtx *pFCA, FTCtx *pFCB, const U8 flash, const int usIvl, const int n)
{
   U8 state;
   for (int i=0; i<n; i++)
   {
      state= i<<4;
      ftdi_write_data(pFCA, &state, sizeof(state));
      state^= flash;
      ftdi_write_data(pFCB, &state, sizeof(state));
      usleep(usIvl);
   }
   state= 0;
   ftdi_write_data(pFCA, &state, sizeof(state));
   ftdi_write_data(pFCB, &state, sizeof(state));
} // flashBang

void devTest1 (const U16 devid, const U8 outMask, const int baudRate, const U8 flags)
{
   FTCtx *pFCA, *pFCB= NULL;
   long usIvl= MICRO_TICKS/18.75;

   pFCA= ftInitUSB(devid, INTERFACE_A, baudRate, flags);
   if (pFCA)
   {
      if ((flags & 0x1) && (ftDevPortCount(devid) > 1)) { pFCB= ftInitUSB(devid, INTERFACE_B, baudRate, flags); }

      if (ftSetModeIF(pFCA, INTERFACE_A, outMask, BITMODE_BITBANG))
      {
         if (ftSetModeIF(pFCB, INTERFACE_B, outMask, BITMODE_BITBANG))
         {
            printf("flashBang2(A,B) ...\n");
            flashBang2(pFCA, pFCB, outMask, usIvl, 100);
         }
         else
         {
            printf("flashBang(A) ...\n");
            flashBang(pFCA, outMask, usIvl, 100);
         }
         printf("... usleep() ...\n");
         usleep(usIvl);
      }
      pFCA= ftCleanup(pFCA, 0x3);
      pFCB= ftCleanup(pFCB, 0x3);
   }
} // devTest1

// Unclear how to make "ftdi_eeprom" utility burn a previously unitialised
// EEPROM ( perhaps multiple flags e.g. --erase-eeprom --build-eeprom
// --flash-eeprom would work? )
// However, the following hack seems to work well...
void burnEEPROM (const U16 devid, char m[], char p[], char s[])
{
   FTCtx *pFC= ftInitUSB(devid, 0, 0, 0);
   if (pFC)
   {
      const char fmt[]= "INFO: ..%s() - r=%d -> %s\n";
      int r;
      r= ftdi_erase_eeprom(pFC);
      printf(fmt, "erase", r, ftdi_get_error_string(pFC));
      r= ftdi_eeprom_initdefaults(pFC,m,p,s);
      printf(fmt, "init", r, ftdi_get_error_string(pFC));
      r= ftdi_eeprom_build(pFC);
      printf(fmt, "build", r, ftdi_get_error_string(pFC));
      r= ftdi_write_eeprom(pFC);
      printf(fmt, "write", r, ftdi_get_error_string(pFC));
      r= ftdi_eeprom_decode(pFC, 1);
      printf(fmt, "decode", r, ftdi_get_error_string(pFC));
      pFC= ftCleanup(pFC, 0);
      pFC= ftCleanup(pFC, 0);
   }
} // burnEEPROM

int main (int argc, char *argv[])
{
   U16 id= ID_PROD_FT2232;
   //burnEEPROM(ID_PROD_FT232, "DR","232RL","1");
   //burnEEPROM(ID_PROD_FT2232, "18069A_P26", "2232H", "191216");
   // error report: "Note: bitbang baudrates are automatically multiplied by 4"
   // 4 ? not 16 ? experiment needed....
   // 300 seems to be lowest supported rate (4.8kHz bit-bang = 18.75 * 256B)
   devTest1(id, 0xFF, 300, 0xF0);
   return(0);
} // main
