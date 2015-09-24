
#include <config.h>
#include <string.h>

#include <grlib/grlib.h>
#include <os/messages.h>

#include "dialogs.h"


/* ===== String Editor ===== */

typedef struct {
	char *			title;
	char *			buffer;
	unsigned int	maxlen;
	DialogCallback	callback;
	void *			p;
} StringDialog;

static StringDialog dlg_getstring;


static void dlgStringRedraw(StringDialog *dlg, rect_t *rect)
{
	void *font;

	/* Dialog frame */
	grFillRect(rect->x, rect->y, rect->x + rect->w, rect->y + rect->h, GR_COLOR_BLACK);
	grFillRect(rect->x + 1, rect->y + 1, rect->x + rect->w - 1, rect->y + rect->h - 1, GR_COLOR_WHITE);

	/* Title */
	font = grLoadFont(GR_FONT_NORMAL);
	grTextOut(rect, font, 8, 5, GR_COLOR_BLACK, dlg->title);

	/* Edit frame */
	grFillRect(rect->x + 8, rect->y + 38, rect->x + rect->w - 8, rect->y + 58, GR_COLOR_GRAY);
	grFillRect(rect->x + 9, rect->y + 39, rect->x + rect->w - 9, rect->y + 57, GR_COLOR_WHITE);

	/* String */
	font = grLoadFont(GR_FONT_BIG);
	grTextOut(rect, font, 10, 41, GR_COLOR_BLACK, dlg->buffer);
}

static void dlgStringHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	int l;
	rect_t rect;
	StringDialog *dlg = (StringDialog *)p;

	switch (msgCode) {
		case MSG_REDRAW:
			rect.x = ((rect_t *)msgPtr)->x + 5;
			rect.y = ((rect_t *)msgPtr)->y + ((rect_t *)msgPtr)->h - 75;
			rect.w = ((rect_t *)msgPtr)->w - 10;
			rect.h = 70;
			dlgStringRedraw(dlg, &rect);
			break;

		case MSG_KEY_PRESSED:
			if (msgParam == 'C') {
				msgUnregisterWindow(window);
				if (dlg->callback) dlg->callback(DLG_CANCEL, dlg->buffer, dlg->p);
			}
			if (msgParam == 'R') {
				msgUnregisterWindow(window);
				if (dlg->callback) dlg->callback(DLG_OK, dlg->buffer, dlg->p);
			}
			if ( (msgParam >= '0') && (msgParam <= '9') ) {
				l = strlen(dlg->buffer);
				if (l >= (dlg->maxlen-1)) break;
				dlg->buffer[l] = msgParam;
				dlg->buffer[l+1] = 0;
				msgInvalidateWindow(window);
			}
			if (msgParam == '#') {
				l = strlen(dlg->buffer);
				if (l >= (dlg->maxlen-1)) break;
				dlg->buffer[l] = '.';
				dlg->buffer[l+1] = 0;
				msgInvalidateWindow(window);
			}
			if (msgParam == '*') {
				l = strlen(dlg->buffer);
				if (l) {
					dlg->buffer[l-1] = 0;
					msgInvalidateWindow(window);
				}
			}
			break;

		case MSG_LONG_KEYPRESS:
			if (msgParam == '*') {
				dlg->buffer[0] = 0;
				msgInvalidateWindow(window);
			}
			break;
	}
}

int dlgGetString(char *title, char *buffer, unsigned int maxlen, DialogCallback cb, void *p)
{
	void *wnd;
	StringDialog *sdlg;

	/* Save dialog parameters */
	sdlg = &dlg_getstring;
	sdlg->title = title;
	sdlg->buffer = buffer;
	sdlg->maxlen = maxlen;
	sdlg->callback = cb;
	sdlg->p = p;

	/* Create dialog window */
	wnd = msgRegisterWindow(title, 0, dlgStringHandler, sdlg);
	if (!wnd) return 0;

	return 1;
}
