
#include <config.h>
#include <stdlib.h>

#include <grlib/grlib.h>
#include <os/messages.h>

#include "window.h"


/* ===== Exported functions ===== */

void wndDrawWindowFrame(void *window)
{
	window_t *wnd;
	rect_t *rect;
	void *font;
	int fh, fw;

	if (!window) return;

	wnd = (window_t *)window;
	rect = &wnd->rect;

	/* Clear screen */
	grFillRect(rect->x, rect->y, rect->x + rect->w, rect->y + rect->h, GR_COLOR_WHITE);

	/* Window title */
	font = grLoadFont(GR_FONT_NORMAL);
	fh = grTextHeight(font);
	fw = grTextWidth(font, wnd->title);
	grFillRect(rect->x, rect->y, rect->x + fw + 2, rect->y + fh + 1, GR_COLOR_GRAY);
	grTextOut(rect, font, 1, 1, GR_COLOR_WHITE, wnd->title);
}

void wndDrawSoftkey(void *window, int type, char *name)
{
	window_t *wnd;
	void *font;
	int fw, fh;

	if (!window) return;

	wnd = (window_t *)window;
	font = grLoadFont(GR_FONT_BIG);
	fh = grTextHeight(font);
	fw = grTextWidth(font, name);

	switch (type) {
		case SOFTKEY_LEFT:
			grTextOut(NULL, font, wnd->rect.x + 2, wnd->rect.y + wnd->rect.h - fh,
				GR_COLOR_BLACK, name);
			break;

		case SOFTKEY_RIGHT:
			grTextOut(NULL, font, wnd->rect.x + wnd->rect.w - 2 - fw, wnd->rect.y + wnd->rect.h - fh,
				GR_COLOR_BLACK, name);
			break;
	}
}
