
#include <config.h>
#include <board.h>
#include <string.h>

#include "eeprom.h"


#define TWI_TIMEOUT		50000

/* ===== Exported functions ===== */

int eepromBlockRead(unsigned char *buf, unsigned int addr, unsigned short size)
{
	int len, tc;
	unsigned int status;

	/* Read, 2-byte internal address */
	AT91C_BASE_TWI->TWI_MMR = AT91C_TWI_MREAD | (2 << 8);

	/* Internal address */
	AT91C_BASE_TWI->TWI_IADR = addr;

	/* Start */
	AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START;

	/* Read bytes */
	len = size;
	while (len--) {
		/* Wait for receiver ready */
		tc = 0;
		do {
			status = AT91C_BASE_TWI->TWI_SR;
			if ( (status & AT91C_TWI_NACK) ) return 0;
			if (tc++ > TWI_TIMEOUT) return 0;
		} while ( !(status & AT91C_TWI_RXRDY) );

		/* Store byte */
		*buf++ = AT91C_BASE_TWI->TWI_RHR;

		/* Stop if one byte remaining */
		if (len == 0) AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;
	}

	/* Wait for transmission complete */
	while ( !(AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXCOMP) );

	return size;
}

int eepromBlockWrite(unsigned char *buf, unsigned int addr, unsigned short size)
{
	int len, tc;
	unsigned int status;

	/* Write, 2-byte internal address */
	AT91C_BASE_TWI->TWI_MMR = (2 << 8);

	/* Internal address */
	AT91C_BASE_TWI->TWI_IADR = addr;

	/* Write bytes */
	len = size;
	while (len--) {
		/* Wait for transmitter ready */
		tc = 0;
		do {
			status = AT91C_BASE_TWI->TWI_SR;
			if ( (status & AT91C_TWI_NACK) ) return 0;
			if (tc++ > TWI_TIMEOUT) return 0;
		} while ( !(status & AT91C_TWI_TXRDY) );

		/* Send byte */
		AT91C_BASE_TWI->TWI_THR = *buf++;
	}

	/* Wait for transmission complete */
	while ( !(AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXCOMP) );

	return size;
}

void i2cInit()
{
	/* Enable TWI clock */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TWI);

	/* Reset TWI  */
	AT91C_BASE_TWI->TWI_CR = AT91C_TWI_SWRST;

	/* Enable master mode */
	AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN;

	/* /32 dividers, SCL = 48 MHz / 70 */
	AT91C_BASE_TWI->TWI_CWGR = 0x2020;

	/* Enable TWI pins */
	AT91C_BASE_PIOA->PIO_ASR = (1 << 10) | (1 << 11);
	AT91C_BASE_PIOA->PIO_PDR = (1 << 10) | (1 << 11);
	AT91C_BASE_PIOA->PIO_MDER = (1 << 10) | (1 << 11);
}
