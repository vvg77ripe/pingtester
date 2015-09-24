
#ifndef _AUDIO_H
#define _AUDIO_H

void audInit(void);
void audShutdown(void);

void audTone(unsigned int freq, unsigned int level, unsigned int time);
void audStop(void);

#endif
