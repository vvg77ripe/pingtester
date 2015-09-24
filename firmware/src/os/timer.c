
#include <config.h>
#include <string.h>

#include <os/messages.h>

#include "timer.h"


#define MAX_TIMERS		16

struct timer {
	void *			window;
	unsigned int	time;
	unsigned int	remain;
	unsigned int	flags;
	unsigned int	id;
};

static struct timer timers[MAX_TIMERS];

/* ===== Exported functions ===== */

void tmrInit()
{
}

void tmrClock(unsigned int elapsed)
{
	int i;
	struct timer *tmr;

	for (i = 0; i < MAX_TIMERS; i++) {
		tmr = &timers[i];
		/* Skip unused entries */
		if ( !(tmr->flags & TMR_FLAG_USED) ) continue;

		/* Generate events */
		if (tmr->remain && (tmr->remain <= elapsed)) {
			msgPostMessage(tmr->window, MSG_TIMER, tmr->id, NULL);
			/* Restart timer */
			tmr->remain = tmr->time;
		}

		/* Decrement remain time */
		if (tmr->remain > elapsed) tmr->remain -= elapsed;
	}
}

void tmrRegisterTimer(void *window, unsigned int time, unsigned int flags, unsigned int id)
{
	int i;
	struct timer *tmr;

	for (i = 0; i < MAX_TIMERS; i++) {
		tmr = &timers[i];
		if (tmr->flags & TMR_FLAG_USED) continue;

		tmr->window = window;
		tmr->time = time;
		tmr->remain = time;
		tmr->id = id;
		tmr->flags = flags | TMR_FLAG_USED;
		break;
	}
}

void tmrDestroyTimer(void *window, unsigned int id)
{
	int i;
	struct timer *tmr;

	for (i = 0; i < MAX_TIMERS; i++) {
		tmr = &timers[i];
		/* Skip unused entries */
		if ( !(tmr->flags & TMR_FLAG_USED) ) continue;

		if ( (tmr->window == window) && (tmr->id == id) ) {
			tmr->flags = 0;
			break;
		}
	}
}
