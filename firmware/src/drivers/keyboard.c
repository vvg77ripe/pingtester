
#include <config.h>
#include <board.h>
#include <stdio.h>
#include <string.h>

#include <drivers/audio.h>
#include <os/messages.h>

#include "keyboard.h"


/* ===== Keyboard connected to port B ===== */

#define KBD_COLS	3
#define KBD_ROWS	5

#define KBD_PIN_COL1	(1 << 25)
#define KBD_PIN_COL2	(1 << 24)
#define KBD_PIN_COL3	(1 << 23)
#define KBD_PIN_COLS	KBD_PIN_COL1 | KBD_PIN_COL2 | KBD_PIN_COL3

#define KBD_PIN_ROW1	(1 << 22)
#define KBD_PIN_ROW2	(1 << 18)
#define KBD_PIN_ROW3	(1 << 19)
#define KBD_PIN_ROW4	(1 << 20)
#define KBD_PIN_ROW5	(1 << 21)
#define KBD_PIN_ROWS	KBD_PIN_ROW1 | KBD_PIN_ROW2 | KBD_PIN_ROW3 | KBD_PIN_ROW4 | KBD_PIN_ROW5


static const unsigned char kbd_codes[KBD_ROWS][KBD_COLS] = {
	{'C', 'M', 'R'},
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}
};

static unsigned char scan_col;
static unsigned char scan_code;
static unsigned char code_time;
static unsigned char code;

/* ===== Exported functions ===== */


void kbdInit()
{
	/* Configure keyboard pins */
	AT91C_BASE_PIOB->PIO_PER = KBD_PIN_COLS | KBD_PIN_ROWS;
	AT91C_BASE_PIOB->PIO_SODR = KBD_PIN_COLS;
	AT91C_BASE_PIOB->PIO_OER = KBD_PIN_COLS;
	AT91C_BASE_PIOB->PIO_PPUER = KBD_PIN_ROWS;
	AT91C_BASE_PIOB->PIO_ODR = KBD_PIN_ROWS;
	
	/* Start scanning at col 1 */
	AT91C_BASE_PIOB->PIO_CODR = KBD_PIN_COL1;
	scan_col = 0;
	code = 0;
}

void kbdScan()
{
	unsigned int status;
	
	/* Read pin status */
	status = AT91C_BASE_PIOB->PIO_PDSR;

	/* Reset new scan code if in first col */
	if (!scan_col) code = 0;

	/* Convert pin status to keycode */
	if ( !(status & KBD_PIN_ROW1) ) code = kbd_codes[0][scan_col];
	if ( !(status & KBD_PIN_ROW2) ) code = kbd_codes[1][scan_col];
	if ( !(status & KBD_PIN_ROW3) ) code = kbd_codes[2][scan_col];
	if ( !(status & KBD_PIN_ROW4) ) code = kbd_codes[3][scan_col];
	if ( !(status & KBD_PIN_ROW5) ) code = kbd_codes[4][scan_col];
	
	/* Scan next row */
	scan_col++;
	if (scan_col >= KBD_COLS) scan_col = 0;
	
	AT91C_BASE_PIOB->PIO_SODR = KBD_PIN_COLS;
	if (scan_col == 0) AT91C_BASE_PIOB->PIO_CODR = KBD_PIN_COL1;
	if (scan_col == 1) AT91C_BASE_PIOB->PIO_CODR = KBD_PIN_COL2;
	if (scan_col == 2) AT91C_BASE_PIOB->PIO_CODR = KBD_PIN_COL3;

	if (scan_col) return;
	
	/* No change */
	if (code == scan_code) {
		if (code) {
			if (code_time < 250) code_time++;
			if (code_time == 10) msgPostMessage(NULL, MSG_LONG_KEYPRESS, code, NULL);
		}
		return;
	}
	
	/* Key released */
	if (code == 0) {
		scan_code = 0;
		return;
	}
	
	/* Two or more keys pressed - ignore */
	if (scan_code != 0) return;
	
	/* Generate event */
	scan_code = code;
	code_time = 0;
	msgPostMessage(NULL, MSG_KEY_PRESSED, code, NULL);

	/* Tone */
	audTone(1000, 20, 100);
}
