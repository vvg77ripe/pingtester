
#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <grlib/grlib.h>

void idle(void);

#define MSG_KEY_PRESSED		0x0001
#define MSG_LONG_KEYPRESS	0x0002
#define MSG_REDRAW			0x0003
#define MSG_MENUCLICK		0x0004
#define MSG_TIMER			0x0005
#define MSG_INIT			0x0006
#define MSG_DESTROY			0x0007


#define WIN_FLAG_USED		0x0001
#define WIN_FLAG_HIDDEN		0x0002


typedef void (*msgHandler)(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p);

typedef struct window window_t;
struct window {
	unsigned int		flags;
	window_t *			next;
	char *				title;
	msgHandler			handler;
	void *				p;
	rect_t				rect;
};


void msgPostMessage(void *msgWindow, unsigned short msgCode, unsigned short msgParam, void *msgPtr);
void msgProcessMessage(void *msgWindow, unsigned short msgCode, unsigned short msgParam, void *msgPtr);

void *msgRegisterWindow(char *title, unsigned int flags, msgHandler handler, void *p);
void msgUnregisterWindow(void *window);
void msgInvalidateWindow(void *window);

#endif
