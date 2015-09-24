//------------------------------------------------------------------------------
//      Headers
//------------------------------------------------------------------------------

#include <config.h>
#include <board.h>
#include <stdio.h>
#include <string.h>
#include <pio/pio.h>
#include <pio/pio_it.h>
#include <aic/aic.h>
#include <utility/trace.h>

#include <drivers/keyboard.h>
#include <drivers/ethernet.h>
#include <drivers/display.h>
#include <drivers/sdcard.h>
#include <drivers/audio.h>
#include <drivers/eeprom.h>
#include <grlib/grlib.h>
#include <usb/usbdevices.h>
#include <net/bridge.h>

#include <os/messages.h>
#include <os/malloc.h>
#include <os/timer.h>
#include <registry.h>
#include <gui.h>


/* ISR_Timer0()
 *   Handles interrupts from Timer0
 * 30 interrupts per second.
 */
static void ISR_Timer0()
{
	unsigned int status = AT91C_BASE_TC0->TC_SR;

	if (status & AT91C_TC_CPCS) {
		/* Keyboard scan */
		kbdScan();

		/* Process timers */
		tmrClock(33);

		/* Redraw display if needed */
		grDisplayRedraw();
	}
}

//------------------------------------------------------------------------------
// Exception handlers
//------------------------------------------------------------------------------

/* Buffer used in exception handlers */
static char exbuf[50];

void exUndefined()
{
	grFillScreen(GR_COLOR_BLUE);
	grTextOut(NULL, grLoadFont(GR_FONT_NORMAL), 10, 50, GR_COLOR_WHITE, "ILLEGAL INSTRUCTION");
	grDisplayRedraw();

	while (1);
}

void exNoEXEC()
{
	grFillScreen(GR_COLOR_BLUE);
	grTextOut(NULL, grLoadFont(GR_FONT_NORMAL), 10, 50, GR_COLOR_WHITE, "CODE ACCESS VIOLATION");
	grDisplayRedraw();

	while (1);
}

void exNoDATA()
{
	register unsigned int *link_ptr;

	__asm__ __volatile__ (
		"sub lr, lr, #8\n"
		"mov %0, lr" : "=r" (link_ptr)
	);

	sprintf(exbuf, "AT %X", (unsigned int)link_ptr);

	grFillScreen(GR_COLOR_BLUE);
	grTextOut(NULL, grLoadFont(GR_FONT_NORMAL), 10, 10, GR_COLOR_WHITE, "DATA ACCESS VIOLATION");
	grTextOut(NULL, grLoadFont(GR_FONT_NORMAL), 10, 25, GR_COLOR_WHITE, exbuf);
	grDisplayRedraw();

	while (1);
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------

static char c_count;

static void FASTCODE ISR_shutdown()
{
	unsigned int status = AT91C_BASE_TC0->TC_SR;

	if (status & AT91C_TC_CPCS) {
		if (AT91C_BASE_PIOB->PIO_PDSR & (1 << 22)) {
			c_count = 0;
		} else {
			c_count++;
			if (c_count >= 2) AT91C_BASE_RSTC->RSTC_RCR = (0xA5 << 24) | 0x05;
		}
	}
}

void FASTCODE shutdown()
{
	/* Shutdown external interfaces */
	usbShutdown();
	sdcShutdown();
	EthShutdown();

	audShutdown();

	/* Disable all interrupts */
	AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF;

	/* Display OFF */
	DISP_OFF();

	/* Disable all peripheral clocks */
	AT91C_BASE_PMC->PMC_PCDR = 0xFFFFFFFF;

	/* Select slow clock, 512 Hz MCK */
	AT91C_BASE_PMC->PMC_MCKR = 0x18;

	/* Disable main oscillator */
	AT91C_BASE_PMC->PMC_MOR &= ~AT91C_CKGR_MOSCEN;

	/* Set low power mode */
	AT91C_BASE_VREG->VREG_MR = 1;

	/* Select keyboard col 1 */
	AT91C_BASE_PIOB->PIO_CODR = (1 << 25);
	AT91C_BASE_PIOB->PIO_SODR = (1 << 23) | (1 << 24);

	/* Start 1-second timer */
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC0) | (1 << AT91C_ID_PIOB);
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
    AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV1_CLOCK
                             | AT91C_TC_WAVESEL_UP_AUTO
                             | AT91C_TC_WAVE;
    AT91C_BASE_TC0->TC_RC = 128;
    AIC_ConfigureIT(AT91C_ID_TC0, 6, ISR_shutdown);
    AIC_EnableIT(AT91C_ID_TC0);
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;

	/* IDLE */
	while (1) AT91C_BASE_PMC->PMC_SCDR = 1;
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main()
{
	// Initialize memory manager
	memInit();

	// Load device configuration
	i2cInit();
	regInit();

	// Initialize display
	DISP_ON();

    // Configure timer 0
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC0);
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
    AT91C_BASE_TC0->TC_IDR = 0xFFFFFFFF;
    AT91C_BASE_TC0->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK
                             | AT91C_TC_WAVESEL_UP_AUTO
                             | AT91C_TC_WAVE;
    AT91C_BASE_TC0->TC_RC = 46875 / 30;
    AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;
    AIC_ConfigureIT(AT91C_ID_TC0, 6, ISR_Timer0);
    AIC_EnableIT(AT91C_ID_TC0);
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;

	/* Configure real-time clock */
	AT91C_BASE_RTTC->RTTC_RTMR = AT91C_RTTC_RTTRST | 32;

	tmrInit();

	// Initialize ethernet controller
	briInit();
	EthInit();

	// Initialize SD card interface
	sdcInit();

	// Initialize keyboard
	kbdInit();

	// Initialize audio interface
	audInit();

	// Initialize USB
	usbInit();

	/* Start GUI */
	guiStart();

    // Driver loop
    while (1) {
		// usb polling
		usbPoll();

		// idle
		idle();
    }
}
