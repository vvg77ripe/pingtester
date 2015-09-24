
#include <config.h>
#include <board.h>
#include <string.h>
#include <stdio.h>

#include <os/messages.h>
#include <memories/Media.h>

#include "sdcard.h"

/* ===== SD Card Interface =====
 *
 * Pinouts
 *
 *   PA12    CS
 *   PA13    SW
 *   PA16    MISO
 *   PA17    MOSI
 *   PA18    MCK
 */

#define SDC_PIN_CS				(1 << 12)
#define SDC_PIN_SW				(1 << 13)
#define SDC_PIN_MISO			(1 << 16)
#define SDC_PIN_MOSI			(1 << 17)
#define SDC_PIN_MCK				(1 << 18)

#define SDC_PIN_MASK			SDC_PIN_CS | SDC_PIN_MOSI | SDC_PIN_MISO | SDC_PIN_MCK;

#define SDC_CLOCK_SLOW			255
#define SDC_CLOCK_FAST			2

/* Timeouts */
#define SD_TIMEOUT_INIT			4096
#define SD_TIMEOUT_DATA			1024
#define SD_TIMEOUT_WRITE		100000

/* SD card commands */
#define SD_CMD_GO_IDLE			0		/* Reset card to IDLE state */
#define SD_CMD_SEND_IF_COND		8		/* Send Interface Condition */
#define SD_CMD_SEND_CSD			9		/* Read Card Specific Data */
#define SD_CMD_SEND_CID			10		/* Read Card Identification Data */
#define SD_CMD_READ_BLOCK		17
#define SD_CMD_WRITE_BLOCK		24
#define SD_CMD_SEND_OP_COND		41		/* Activate card */
#define SD_CMD_APP				55		/* Application specific prefix */

/* SD card R1 bit maps */
#define SD_R1_IDLE				0x01
#define SD_R1_ERASE_RESET		0x02
#define SD_R1_ILLEGAL_COMMAND	0x04
#define SD_R1_CRC_ERROR			0x08
#define SD_R1_SEQUENCE_ERROR	0x10
#define SD_R1_ADDRESS_ERROR		0x20
#define SD_R1_PARAMETER_ERROR	0x40


/* ===== Card Data ===== */

static sdcard card;
static Media cardmedia;


/* ===== Internal functions ===== */

/* WARNING
 *   Most of the sd card functions MUST be located in RAM
 * (Must have inline or FASTCODE in declaration), because they are used for
 * firmware upgrade.
 */

static inline void sdcSetSpeed(unsigned char speed)
{
	/* 8 bit, preserve chip select */
	AT91C_BASE_SPI0->SPI_CSR[0] = AT91C_SPI_CPOL | AT91C_SPI_BITS_8 | AT91C_SPI_CSAAT | (speed << 8);
}

static FASTCODE unsigned char sdcSendByte(unsigned char byte)
{
	/* Wait for previous transfer complete */
	while ( !(AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TDRE) );

	/* Send byte */
	AT91C_BASE_SPI0->SPI_TDR = byte;

	/* Wait for char received */
	while ( !(AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF) );

	return AT91C_BASE_SPI0->SPI_RDR;
}

static inline unsigned char sdcRecvByte()
{
	return sdcSendByte(0xFF);
}

static inline void sdcReleaseCS()
{
	sdcSendByte(0xFF);
	AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_LASTXFER;
}

static inline unsigned char sdcReady()
{
	int n;

	for (n = 0; n < 1000; n++) {
		if (sdcRecvByte() == 0xFF) return 1;
	}

	return 0;
}

static unsigned char sdcRecvR1()
{
	unsigned char res;
	int n;

	/* Wait for response */
	for (n = 0; n < 10; n++) {
		res = sdcRecvByte();
		if (res != 0xFF) break;
	}

	return res;
}

static FASTCODE unsigned char sdcCommand(unsigned char code, unsigned int arg)
{
	unsigned char res, crc;
	int n;

	/* Wait for card ready */
	if (!sdcReady()) return 0xFF;

	/* Send command */
	sdcSendByte(0x40 | code);
	sdcSendByte((arg >> 24) & 0xFF);
	sdcSendByte((arg >> 16) & 0xFF);
	sdcSendByte((arg >> 8) & 0xFF);
	sdcSendByte(arg & 0xFF);
	crc = 0;
	if (code == 0) crc = 0x95;
	if (code == 8) crc = 0x87;
	sdcSendByte(crc);

	/* Wait for response */
	for (n = 0; n < 10; n++) {
		res = sdcRecvByte();
		if (!(res & 0x80)) break;
	}

	return res;
}

static FASTCODE int sdcDataRead(unsigned char *buffer, unsigned short size)
{
	int i;
	unsigned char res;

	/* Wait for data token */
	for (i = 0; i < SD_TIMEOUT_DATA; i++) {
		res = sdcRecvByte();
		if (res != 0xFF) break;
	}

	/* Check for valid token */
	if (res != 0xFE) return 0;

	/* Read data bytes */
	for (i = 0; i < size; i++) *(buffer++) = sdcRecvByte();

	/* Skip CRC */
	sdcRecvByte();
	sdcRecvByte();

	return 1;
}

static void sdcCalcSize()
{
	unsigned char csdver;
	unsigned int c_size, c_size_mult, read_bl_len;

	csdver = (card.csd[0] >> 6);

	/* Version 1.0 structure */
	if (csdver == 0) {
		read_bl_len = card.csd[5] & 0x0F;
		c_size = (card.csd[8] >> 6) | (card.csd[7] << 2) | ((card.csd[6] & 3) << 10);
		c_size_mult = (card.csd[10] >> 7) | ((card.csd[9] & 3) << 1);
		card.size = (c_size + 1) * (1 << (c_size_mult+2));
		/* Adjust size regard to block len */
		if (read_bl_len == 9) card.size = card.size >> 1;
		if (read_bl_len == 11) card.size = card.size << 1;
	}

	/* Version 2.0 structure */
	if (csdver == 1) {
		card.hc = 1;
		c_size = card.csd[9] | (card.csd[8] << 8) | (card.csd[7] << 16);
		card.size = (c_size + 1) << 9;
	}
}

/* ===== Exported functions ===== */

void sdcInit()
{
	/* Configure SD card pins */
	AT91C_BASE_PIOA->PIO_ASR = SDC_PIN_MASK;
	AT91C_BASE_PIOA->PIO_PDR = SDC_PIN_MASK;

	/* Configure SPI controller */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0);
	/* SPI enable and reset */
	AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN | AT91C_SPI_SWRST;
	/* Master mode, CS0 active */
	AT91C_BASE_SPI0->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS | (0xE << 16);
	/* SPI enable  */
	AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN;
}

void sdcShutdown()
{
	/* SPI disable */
	AT91C_BASE_SPI0->SPI_CR = 0;

	/* Set all control lines low */
	AT91C_BASE_PIOA->PIO_CODR = SDC_PIN_CS | SDC_PIN_MOSI | SDC_PIN_MCK;
	AT91C_BASE_PIOA->PIO_OER = SDC_PIN_CS | SDC_PIN_MOSI | SDC_PIN_MCK;
	AT91C_BASE_PIOA->PIO_PER = SDC_PIN_CS | SDC_PIN_MOSI | SDC_PIN_MCK;
}

int sdcCardActivate()
{
	int i;
	unsigned char res;

	/* Clean old data */
	memset(&card, 0, sizeof(card));
	card.state = CARD_STATE_INIT;

	/* Set slow speed */
	sdcSetSpeed(SDC_CLOCK_SLOW);

	/* Send CLK pulses to card for internal initialization */
	for (i = 0; i < 10; i++) sdcRecvByte();

	/* Disable the CS line */
	AT91C_BASE_PIOA->PIO_PER = SDC_PIN_CS;
	AT91C_BASE_PIOA->PIO_SODR = SDC_PIN_CS;
	AT91C_BASE_PIOA->PIO_OER = SDC_PIN_CS;
	/* Wait */
	for (i = 0; i < 2; i++) sdcRecvByte();
	/* Re-enable the CS line */
	AT91C_BASE_PIOA->PIO_PDR = SDC_PIN_CS;

	do {
		/* Reset the card */
		res = sdcCommand(SD_CMD_GO_IDLE, 0);
		if (res & 0x80) break;

		/* Send interface condition */
		res = sdcCommand(SD_CMD_SEND_IF_COND, 0x01AA);

		/* If accepted, we have a v2.0 SD card */
		card.ver = 1;
		if (res == 1) {
			card.ver = 2;
			/* Skip actual response */
			sdcRecvByte();
			sdcRecvByte();
			sdcRecvByte();
			sdcRecvByte();
		}

		/* Activate the card, enable high capacity, wait for initialization */
		for (i = 0; i < SD_TIMEOUT_INIT; i++) {
			/* Lets perform other tasks */
			idle();

			/* Send ACMD41 */
			res = sdcCommand(SD_CMD_APP, 0);
			if (res & 0x80) continue;
			res = sdcCommand(SD_CMD_SEND_OP_COND, (1 << 30));
			if (res == 0) break;
		}

		/* Break if timeout */
		if (i >= SD_TIMEOUT_INIT) break;

		/* Set normal speed */
		sdcSetSpeed(SDC_CLOCK_FAST);

		/* Read CSD */
		if (sdcCommand(SD_CMD_SEND_CSD, 0) != 0) break;
		if (!sdcDataRead(card.csd, 16)) break;

		/* Read CID */
		if (sdcCommand(SD_CMD_SEND_CID, 0) != 0) break;
		if (!sdcDataRead(card.cid, 16)) break;

		/* Calculate size of the card */
		sdcCalcSize();

		/* Card ready */
		card.state = CARD_STATE_READY;

		/* OK */
		sdcReleaseCS();
		return 1;
	} while (0);

	/* Release chip select*/
	sdcReleaseCS();

	/* FAIL */
	card.state = CARD_STATE_UNUSABLE;
	return 0;
}

sdcard * sdcCardInfo()
{
	return &card;
}

int FASTCODE sdcBlockRead(unsigned char *buf, unsigned int block, unsigned int count)
{
	unsigned int i;
	unsigned int blk, blkinc;

	/* Check for card is ready */
	if (card.state != CARD_STATE_READY) return 0;

	/* Check for high capacity card */
	if (card.hc) {
		blk = block;
		blkinc = 1;
	} else {
		blk = block * CARD_BLOCK_SIZE;
		blkinc = CARD_BLOCK_SIZE;
	}

	/* Read blocks */
	for (i = 0; i < count; i++) {
		/* Send command */
		if (sdcCommand(SD_CMD_READ_BLOCK, blk) != 0) {
			sdcReleaseCS();
			return 0;
		}

		/* Read block data */
		if (!sdcDataRead(buf, CARD_BLOCK_SIZE)) {
			sdcReleaseCS();
			return 0;
		}

		/* Increment pointers */
		buf += CARD_BLOCK_SIZE;
		blk += blkinc;
	}

	/* OK */
	sdcReleaseCS();
	return 1;
}

int sdcBlockWrite(unsigned char *buf, unsigned int block, unsigned int count)
{
	unsigned int i, b;
	unsigned int blk, blkinc;

	/* Check for card is ready */
	if (card.state != CARD_STATE_READY) return 0;

	/* Check for high capacity card */
	if (card.hc) {
		blk = block;
		blkinc = 1;
	} else {
		blk = block * CARD_BLOCK_SIZE;
		blkinc = CARD_BLOCK_SIZE;
	}

	/* Write blocks */
	for (i = 0; i < count; i++) {
		/* Send command */
		if (sdcCommand(SD_CMD_WRITE_BLOCK, blk) != 0) {
			sdcReleaseCS();
			return 0;
		}

		/* Write block data */
		sdcSendByte(0xFF);
		sdcSendByte(0xFE);
		for (b = 0; b < CARD_BLOCK_SIZE; b++) {
			sdcSendByte(*buf++);
		}

		/* Check data response token */
		if ( (sdcRecvR1() & 0x1F) != 0x05) {
			sdcReleaseCS();
			return 0;
		}

		/* Wait for card ready */
		for (b = 0; b < SD_TIMEOUT_WRITE; b++) {
			if (sdcRecvByte() == 0xFF) break;
		}
		if (b == SD_TIMEOUT_WRITE) {
			sdcReleaseCS();
			return 0;
		}

		/* Increment pointers */
		blk += blkinc;
	}

	/* OK */
	sdcReleaseCS();
	return 1;
}

/* ===== Media interface ===== */

static unsigned char card_read(Media *media, unsigned int address, void *data, unsigned int length,
		MediaCallback callback, void *argument)
{
	/* Check card status */
	if (card.state != CARD_STATE_READY) return MED_STATUS_BUSY;

	/* Check media status */
	if (media->state != MED_STATE_READY) return MED_STATUS_BUSY;
	media->state = MED_STATE_BUSY;

	/* Read data from sd card */
	sdcBlockRead(data, address, length);

	media->state = MED_STATE_READY;

	/* Invoke callback, if any */
	if (callback) callback(argument, MED_STATUS_SUCCESS, 0, 0);

	/* OK */
	return MED_STATUS_SUCCESS;
}

static unsigned char card_write(Media *media, unsigned int address, void *data, unsigned int length,
		MediaCallback callback, void *argument)
{
	/* Check card status */
	if (card.state != CARD_STATE_READY) return MED_STATUS_BUSY;

	/* Check media status */
	if (media->state != MED_STATE_READY) return MED_STATUS_BUSY;
	media->state = MED_STATE_BUSY;

	/* Read data from sd card */
	sdcBlockWrite(data, address, length);

	media->state = MED_STATE_READY;

	/* Invoke callback, if any */
	if (callback) callback(argument, MED_STATUS_SUCCESS, 0, 0);

	return MED_STATUS_SUCCESS;
}

Media *sdcGetMedia()
{
	memset(&cardmedia, 0, sizeof(cardmedia));
	cardmedia.read = card_read;
	cardmedia.write = card_write;
	cardmedia.state = MED_STATE_READY;
	cardmedia.size = card.size * 1024;

	return &cardmedia;
}
