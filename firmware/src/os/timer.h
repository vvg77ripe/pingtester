
#ifndef _TIMER_H
#define _TIMER_H

#define TMR_FLAG_USED		0x01
#define TMR_FLAG_SINGLE		0x02

void tmrInit(void);
void tmrClock(unsigned int elapsed);
void tmrRegisterTimer(void *window, unsigned int time, unsigned int flags, unsigned int id);
void tmrDestroyTimer(void *window, unsigned int id);

#endif
