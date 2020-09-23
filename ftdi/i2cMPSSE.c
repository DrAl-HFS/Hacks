//#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpsse.h"

int main (int argc, char *argv[])
{
	int r= -1;
	struct mpsse_context *pI2C= NULL;

   pI2C= MPSSE(I2C, FOUR_HUNDRED_KHZ, MSB);
	if (pI2C && pI2C->open)
	{
		printf("%s initialized at %dHz (I2C)\n", GetDescription(pI2C), GetClock(pI2C));

		if ((MPSSE_OK == Start(pI2C)) && (MPSSE_OK == Write(pI2C, "\x91\x00", 2)))
      {
         if (ACK == GetAck(pI2C))
         {
            char *pDB= Read(pI2C, 2);
            if (pDB)
            {
               r= 1;
               printf("Read() - %02X,%02X\n",pDB[0],pDB[1]);
               free(pDB);
            }
            SendNacks(pI2C);
            Read(pI2C, 1);
         }
         r= Stop(pI2C);
      }
	} else { printf("Failed to initialize MPSSE: %s\n", ErrorString(pI2C)); }
	Close(pI2C);

	return(r);
} // main
