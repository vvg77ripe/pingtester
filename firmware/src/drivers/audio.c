
#include <config.h>
#include <string.h>
#include <board.h>
#include <aic/aic.h>

#include "audio.h"


#define PWM_PERIOD		1024

/* ===== Sample timer interrupt handler ===== */

static unsigned int tone_level;
static unsigned int tone_samples;

static void ISR_Sample()
{
	unsigned int status = AT91C_BASE_TC1->TC_SR;

	if (status & AT91C_TC_CPCS) {
		/* Tone generator */
		if (tone_samples) {
			if (tone_samples & 1) {
				AT91C_BASE_PWMC->PWMC_CH[0].PWMC_CUPDR = (PWM_PERIOD / 2) + tone_level;
			} else {
				AT91C_BASE_PWMC->PWMC_CH[0].PWMC_CUPDR = (PWM_PERIOD / 2) - tone_level;
			}
			tone_samples--;
		}

		/* Stop timer if no audio */
		if (!tone_samples) audStop();
	}
}

/* ===== Exported functions ===== */

void audInit()
{
	/* Configure PWM controller */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PWMC);
	AT91C_BASE_PWMC->PWMC_CH[0].PWMC_CDTYR = (PWM_PERIOD / 2);
	AT91C_BASE_PWMC->PWMC_CH[0].PWMC_CPRDR = PWM_PERIOD;
	AT91C_BASE_PWMC->PWMC_ENA = 1;

	/* Configure sample timer */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC1);
	AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKDIS;
	AT91C_BASE_TC1->TC_IDR = 0xFFFFFFFF;
	AT91C_BASE_TC1->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK
                             | AT91C_TC_WAVESEL_UP_AUTO
                             | AT91C_TC_WAVE;
	AT91C_BASE_TC1->TC_RC = 46875;
	AT91C_BASE_TC1->TC_IER = AT91C_TC_CPCS;
	AIC_ConfigureIT(AT91C_ID_TC1, 7, ISR_Sample);
	AIC_EnableIT(AT91C_ID_TC1);

	/* Enable PWM output */
	AT91C_BASE_PIOB->PIO_PDR = (1 << 27);
	AT91C_BASE_PIOB->PIO_BSR = (1 << 27);
}

void audShutdown()
{
}

void audTone(unsigned int freq, unsigned int level, unsigned int time)
{
	if (!freq) return;

	tone_level = level;
	tone_samples = time * freq / 500;

	AT91C_BASE_TC1->TC_RC = 46875 / freq / 2;
	AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
}

void audStop()
{
	AT91C_BASE_TC1->TC_CCR = 0;
	AT91C_BASE_PWMC->PWMC_CH[0].PWMC_CUPDR = (PWM_PERIOD / 2);
}
