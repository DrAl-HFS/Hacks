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

#define SEC_US 1000000 // microseconds per second

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int    U32;
typedef struct ftdi_context FTCtx;

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
   printf("libftdi V%d.%d.%d\n", v.major, v.minor, v.micro);
   //else { printf("ERROR: ..%s() - r=%d\n", "get_library_version", r); }
} // ftDumpLibInfo

void ftDumpDevInfo (FTCtx *pFC)
{
   //int r=-1;
   //char mfr[64], desc[64], ser[64];
   //r= ftdi_usb_get_strings(pFC, usb_dev???, mfr, sizeof(mfr), desc, sizeof(desc), ser, sizeof(ser));
   //r= ftdi_eeprom_get_strings(pFC, mfr, sizeof(mfr)-1, desc, sizeof(desc)-1, ser, sizeof(ser)-1);
   //if (r >= 0) { printf("M,D,S:-\n%s\n%s\n%s\n",mfr,desc,ser); }

   //U32 id; r= ftdi_read_chipid(&gFC, &id);
   //if (r >= 0) { printf("ID: %X\n", id); }
   //else { printf("ERROR: ..%s() - r=%d\n", "read_chipid", r); }

   printf("DevTyp %d\tbaud=%d\n", pFC->type, pFC->baudrate);
   printf("IF/Idx: %d, %d\n", pFC->interface, pFC->index);
/* Nothing useful happening here...
   r= ftdi_read_eeprom(pFC);
   printf("EEPROM: %d -> %p:\n", r, pFC->eeprom);
   for (int fid= CBUS_FUNCTION_0; fid<=CBUS_FUNCTION_9; fid++)
   {
      int val= 0xA5FFFF00 ^ fid;
      r= ftdi_get_eeprom_value(pFC, fid, &val);
      printf("[%d] - %d - 0x%X\n", fid, r, val);
   }
   printf("\n--\n");

   for (int i= VENDOR_ID; i<=PRODUCT_ID; i++)
   {
      int val=0xA5FFFF00 ^ i;
      //r= ftdi_read_eeprom_location(pFC, i, &val);
      r= ftdi_get_eeprom_value(pFC, i, &val);
      printf("r%d [%d] : %04x\n", r, i, val);
   }
*/
} // ftDumpDevInfo

FTCtx *ftInitUSB (const U16 devid, const enum ftdi_interface ifid, const U8 flags)
{
   FTCtx *pFC= ftdi_new();
   if (pFC)
   {
      int r;

      if (flags & 0x10) { ftDumpLibInfo(); }

      if (ifid > INTERFACE_ANY)
      {  // Multiple interfaces on a device need setup before USB binding (for endpoint definition ?)
         r= ftdi_set_interface(pFC, ifid);
         if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_interface", ifid, r); }
      }

      r= ftdi_usb_open(pFC, ID_VNDR_FTDI, devid);

      if (r < 0) { printf("ERROR: ..%s() - r=%d\n", "usb_open", r); }
      else
      {
         if (flags & 0x01) { ftDumpDevInfo(pFC); }
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
         if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_interface", ifid, r); }
      }
      if (0 != bmid) { ftdi_set_bitmode(pFC, 0, 0); } // reset
      r= ftdi_set_bitmode(pFC, m, bmid);
      if (r < 0) { printf("ERROR: ..%s(%d) -> %d\n", "set_bitmode", bmid, r); }
   }
   return(r >= 0);
} // ftSetModeIF

FTCtx *ftCleanup (FTCtx *pFC)
{
   if (pFC)
   {
      //ftSetModeIF(pFC, 0, 0, BITMODE_RESET);
      int r= ftdi_usb_reset(pFC);
      if (r < 0) { printf("ERROR: ..%s() -> %d\n", "usb_reset", r); }
      r= ftdi_usb_close(pFC);
      if (r < 0) { printf("ERROR: ..%s() -> %d\n", "usb_close", r); }
      ftdi_free(pFC);
   }
   return(NULL);
} // ftCleanup

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

void devTest1 (const U16 devid, const U8 flags)
{
   FTCtx *pFCA, *pFCB= NULL;

   pFCA= ftInitUSB(devid, INTERFACE_A, flags);
   if (pFCA)
   {
      if (ftDevPortCount(devid) > 1) { pFCB= ftInitUSB(devid, INTERFACE_B, 0); }

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
      pFCA= ftCleanup(pFCA);
      pFCB= ftCleanup(pFCB);
   }
} // devTest1

// Unclear how to make "ftdi_eeprom" utility burn a previously unitialised
// EEPROM ( perhaps multiple flags e.g. --erase-eeprom --build-eeprom
// --flash-eeprom would work? )
// However, the following hack seems to work well...
void burnEEPROM (const U16 devid, char m[], char p[], char s[])
{
   FTCtx *pFC= ftInitUSB(devid, 0, 0);
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
      pFC= ftCleanup(pFC);
   }
} // burnEEPROM

int main (int argc, char *argv[])
{
   U16 id= ID_PROD_FT2232;
   //burnEEPROM(ID_PROD_FT232, "DR","232RL","1");
   //burnEEPROM(ID_PROD_FT2232, "18069A_P26", "2232H", "191216");

   devTest1(id, 0x11);
   return(0);
} // main
