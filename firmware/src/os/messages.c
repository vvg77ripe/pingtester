
#include <config.h>
#include <string.h>

#include <grlib/grlib.h>
#include <net/bridge.h>

#include "messages.h"


/* ===== Message queue ===== */

#define MESSAGE_QUEUE_SIZE		16

struct message {
	void *				msgWindow;
	unsigned short		msgCode;
	unsigned short		msgParam;
	void *				msgPtr;
};

static struct message msgQueue[MESSAGE_QUEUE_SIZE];
static unsigned int msgFirst;
static unsigned int msgLast;

#define msgNext(index) ( (index >= (MESSAGE_QUEUE_SIZE-1)) ? 0 : index + 1 )

/* ===== Windows ===== */

#define MAX_WINDOWS				8

static window_t winlist[MAX_WINDOWS];
static window_t *topwindow;

/* ===== Private functions ===== */

static void RedrawTopWindow()
{
	msgPostMessage(NULL, MSG_REDRAW, 0, &topwindow->rect);
}

/* ===== Exported functions ===== */

void idle()
{
	/* Process queued messages */
	while (msgFirst != msgLast) {
		msgProcessMessage(msgQueue[msgFirst].msgWindow,
			msgQueue[msgFirst].msgCode, msgQueue[msgFirst].msgParam, msgQueue[msgFirst].msgPtr);
		msgFirst = msgNext(msgFirst);
	}

	/* Process received packets */
	ifRecvPoll();
}

void msgPostMessage(void *msgWindow, unsigned short msgCode, unsigned short msgParam, void *msgPtr)
{
	/* Check for queue is full */
	if (msgNext(msgLast) == msgFirst) return;

	/* Fill next element in queue */
	msgQueue[msgLast].msgWindow = msgWindow;
	msgQueue[msgLast].msgCode = msgCode;
	msgQueue[msgLast].msgParam = msgParam;
	msgQueue[msgLast].msgPtr = msgPtr;
	msgLast = msgNext(msgLast);
}

void msgProcessMessage(void *msgWindow, unsigned short msgCode, unsigned short msgParam, void *msgPtr)
{
	window_t *wnd;

	/* All messages are sent to top window, unless window handle is specified */
	wnd = topwindow;
	if (msgWindow) wnd = (window_t *)msgWindow;

	/* Check window handle */
	if (!wnd) return;
	if (!wnd->flags) return;
	
	/* Call window message handler */
	if (wnd->handler) wnd->handler(wnd, msgCode, msgParam, msgPtr, wnd->p);
}

void *msgRegisterWindow(char *title, unsigned int flags, msgHandler handler, void *p)
{
	int i;
	window_t *win;
	
	/* Find free entry in the window table */
	win = NULL;
	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].flags) {
			win = &winlist[i];
			break;
		}
	}
	
	/* Max windows */
	if (!win) return NULL;

	/* Fill window structure */
	win->flags = WIN_FLAG_USED | flags;
	win->title = title;
	win->handler = handler;
	win->p = p;

	/* Default window rect */
	win->rect.x = 0;
	win->rect.y = 20;
	win->rect.w = 176;
	win->rect.h = 112;
	
	/* Add to chain */
	win->next = topwindow;
	topwindow = win;

	/* Send init message to handler */
	if (win->handler) win->handler(win, MSG_INIT, 0, NULL, win->p);
	
	/* Display window */
	RedrawTopWindow();
	
	return win;
}

void msgUnregisterWindow(void *window)
{
	if (!window) return;

	/* Send destroy message to handler */
	if ( ((window_t *)window)->handler )
		((window_t *)window)->handler(window, MSG_DESTROY, 0, NULL, ((window_t *)window)->p);

	if (window == topwindow) {
		/* Unregistering top window is simple */
		topwindow->flags = 0;
		topwindow = topwindow->next;
		/* Display next window in chain */
		RedrawTopWindow();
	} else {
	}
}

void msgInvalidateWindow(void *window)
{
	if (window == topwindow) RedrawTopWindow();
}
