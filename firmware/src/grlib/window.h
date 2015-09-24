
#ifndef _WINDOW_H
#define _WINDOW_H

#define SOFTKEY_LEFT	0
#define SOFTKEY_RIGHT	1

void wndDrawWindowFrame(void *window);
void wndDrawSoftkey(void *window, int type, char *name);

#endif
