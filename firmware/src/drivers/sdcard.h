
#ifndef _SDCARD_H
#define _SDCARD_H

#include <memories/Media.h>

#define CARD_STATE_REMOVED		0
#define CARD_STATE_INIT			1
#define CARD_STATE_UNUSABLE		2
#define CARD_STATE_BUSY			3
#define CARD_STATE_READY		4

#define CARD_BLOCK_SIZE			512

typedef struct cdata {
	unsigned char	state;				/* Card state */
	unsigned char	ver;				/* SD card protocol version */
	unsigned char	hc;					/* SDHC capable flag */
	unsigned char	csd[16];			/* Card Specific Data Register */
	unsigned char	cid[16];			/* Card Identification Register */
	unsigned int	size;				/* Card size, in KBytes */
} sdcard;

void sdcInit(void);
void sdcShutdown(void);
int sdcCardActivate(void);
sdcard * sdcCardInfo(void);

int sdcBlockRead(unsigned char *buf, unsigned int block, unsigned int count);
int sdcBlockWrite(unsigned char *buf, unsigned int block, unsigned int count);
Media *sdcGetMedia();

#endif
