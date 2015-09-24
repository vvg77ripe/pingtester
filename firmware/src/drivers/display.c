
#include <config.h>
#include <board.h>
#include <string.h>

#include "drivers/display.h"
#include "grlib/grlib.h"
#include "grlib/menus.h"


/***** Display interface *****
 *
 * Pinouts
 *   PA1         PWR1         2.9V display power ON
 *   PA2         PWR2         White LEDs power ON
 *   PA22        CLK
 *   PA23        DATA
 *   PA24        RS
 *   PA25        RESET
 *   PA26        CS
 */

#define DISP_PIN_PWR1		(1 << 1)
#define DISP_PIN_PWR2		(1 << 2)
#define DISP_PIN_CLK		(1 << 22)
#define DISP_PIN_DATA		(1 << 23)
#define DISP_PIN_RS			(1 << 24)
#define DISP_PIN_RESET		(1 << 25)
#define DISP_PIN_CS			(1 << 26)
#define DISP_PIN_MASK		DISP_PIN_PWR1 | DISP_PIN_PWR2 | DISP_PIN_CLK | DISP_PIN_DATA | \
							DISP_PIN_RS | DISP_PIN_RESET | DISP_PIN_CS


#define DISPLAY_W			176
#define DISPLAY_H			132

#define DISPLAY_CHUNK		4		/* 4 lines */

/***** Display buffer *****/

static unsigned short palette[256];
static unsigned char screen[DISPLAY_W * DISPLAY_H];
static unsigned short lines[DISPLAY_W * DISPLAY_CHUNK];

extern unsigned short testimage[];

/***** Private functions *****/

static __attribute__((noinline)) void disp_1ms()
{
	int i;

	for (i = 0; i < 2000; i++) asm("");
}

static void disp_wait(int time)
{
	int i;

	for (i = 0; i < time; i++) disp_1ms();
}

static void disp_word(unsigned short w)
{
	int i;
	unsigned int x;

	x = w;
	for (i = 0; i < 16; i++) {
		// DATA pin
		(x & 0x8000) ? (*AT91C_PIOA_SODR = DISP_PIN_DATA) : (*AT91C_PIOA_CODR = DISP_PIN_DATA);
		// CLK high
		*AT91C_PIOA_SODR = DISP_PIN_CLK;
		// shift to next bit
		x = x << 1;
		// CLK low
		*AT91C_PIOA_CODR = DISP_PIN_CLK;
	}
}

static FASTCODE void disp_redraw()
{
	int i, x, y, ly;
	unsigned short *dest;
	unsigned char *src;

	/* Assert chip select */
	*AT91C_PIOA_CODR = DISP_PIN_CS;

	for (y = 0; y < DISPLAY_H; y += DISPLAY_CHUNK) {
		ly = DISPLAY_CHUNK;

		/* xlat */
		dest = lines;
		for (i = 0; i < ly; i++) {
			src = &screen[DISPLAY_W * (y + i + 1) - 1];
			for (x = 0; x < DISPLAY_W; x++) *(dest++) = palette[*(src--)];
		}

		/* Write chunk */
		AT91C_BASE_SSC->SSC_TPR = (unsigned int) lines;
		AT91C_BASE_SSC->SSC_TCR = DISPLAY_W * ly;
		AT91C_BASE_SSC->SSC_PTCR = AT91C_PDC_TXTEN;

		/* Wait for write complete */
		while ( !(AT91C_BASE_SSC->SSC_SR & AT91C_SSC_ENDTX) );
	}

	/* Wait for last byte out */
	while ( !(AT91C_BASE_SSC->SSC_SR & AT91C_SSC_TXEMPTY) );

	/* Release chip select */
	*AT91C_PIOA_SODR = DISP_PIN_CS;
}

// ***** Exported functions *****

void DISP_ON()
{
	void *font;

	/* Configure pins */
	*AT91C_PIOA_OER = DISP_PIN_MASK;	/* Set pins as output */
	*AT91C_PIOA_PER = DISP_PIN_MASK;	/* Set PIO mode */
	*AT91C_PIOA_CODR = DISP_PIN_RESET | DISP_PIN_DATA | DISP_PIN_CLK;
	*AT91C_PIOA_SODR = DISP_PIN_RS | DISP_PIN_CS;

	/* Power on the display */
	*AT91C_PIOA_SODR = DISP_PIN_PWR1 | DISP_PIN_PWR2;

	/* Release RESET signal */
	disp_wait(1);
	*AT91C_PIOA_SODR = DISP_PIN_RESET;

	/* Wait for reset complete, prepare to initialize the controller */
	disp_wait(50);
	*AT91C_PIOA_CODR = DISP_PIN_CS;

	/* Software reset */
	disp_word(0xFDFD);
	disp_word(0xFDFD);

	/* Wait 50ms for reset complete */
	disp_wait(50);

	/* Initialize LCD */
	disp_word(0xEF00);
	disp_word(0xEE04);
	disp_word(0x1B04);

	/* Host reset */
	disp_word(0xFEFE);
	disp_word(0xFEFE);

	disp_word(0xEF90);
	disp_word(0x4A04);
	disp_word(0x7F3F);
	disp_word(0xEE04);
	disp_word(0x4306);

	/* Wait 7ms for stabilize glass voltage */
	disp_wait(7);

	/* Continue initialization */
	disp_word(0xEF90);
	disp_word(0x0983);		/* X end position - 131 */
	disp_word(0x0800);		/* X start position - 0 */
	disp_word(0x0BAF);		/* Y end position - 175 */
	disp_word(0x0A00);		/* Y start position - 0 */
	disp_word(0x0504);		/* Orientation */
	disp_word(0x0600);		/* Section X */
	disp_word(0x0700);		/* Section Y */

	disp_word(0xEF00);
	disp_word(0xEE0C);

	disp_word(0xEF90);
	disp_word(0x0080);

	disp_word(0xEFB0);
	disp_word(0x4902);

	disp_word(0xEF00);
	disp_word(0x7F01);
	disp_word(0xE181);
	disp_word(0xE202);
	disp_word(0xE276);
	disp_word(0xE183);

	/* Finishing initialization */
	disp_wait(50);
	disp_word(0x8001);		/* Displaying ON */
	disp_word(0xEF90);
	disp_word(0x0000);

	disp_word(0x0504);

	/* Release chip select */
	*AT91C_PIOA_SODR = DISP_PIN_CS;

	/* For data transfer we need to start the SSC */
	AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SSC;
	AT91C_BASE_SSC->SSC_CMR = 2;								/* 12 MHz SSC clock */
	AT91C_BASE_SSC->SSC_TCMR = AT91C_SSC_CKO_DATA_TX;
	AT91C_BASE_SSC->SSC_TFMR = 15 | AT91C_SSC_MSBF;
	AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXEN;

	AT91C_BASE_PIOA->PIO_PDR = DISP_PIN_CLK | DISP_PIN_DATA;	/* Enable SSC pins */
	AT91C_BASE_PIOA->PIO_ASR = DISP_PIN_CLK | DISP_PIN_DATA;

	/* Data mode */
	*AT91C_PIOA_CODR = DISP_PIN_RS;

	/* Initialize graphic library */
	grDisplayInit(screen, DISPLAY_W, DISPLAY_H, palette, disp_redraw);

	/* Clear screen */
	grFillScreen(GR_COLOR_WHITE);
	grDisplayRedraw();
}

void DISP_OFF()
{
	/* Clear screen */
	grFillScreen(GR_COLOR_WHITE);
	grDisplayRedraw();
	sleep(100);

	// EF00, 7E04, EFB0, 5A48, EF00, 7F01, EFB0, 64FF, 6500, EF00, 7F01, E262, E202, EFB0, BC02,
	// EF00, 7F01, E200, 8000, E204, E200, E100, EFB0, BC00, EF00, 7F01

	/* Power off the display */
	AT91C_BASE_PIOA->PIO_PER = DISP_PIN_MASK;
	AT91C_BASE_PIOA->PIO_CODR = DISP_PIN_MASK;
}

void sleep(int time)
{
	disp_wait(time);
}
