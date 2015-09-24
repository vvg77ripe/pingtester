
#include <config.h>
#include <string.h>

#include <fs/romfs.h>
#include <grlib/grlib.h>
#include <os/messages.h>

#include "menus.h"

#define GR_MENUITEM_WIDTH		32
#define GR_MENUITEM_HEIGHT		32
#define GR_MENUITEM_FONT		GR_FONT_NORMAL

#define GR_MENU_COLS			3
#define GR_MENU_ROWS			2

static const unsigned int menu_cols[GR_MENU_COLS] = {15, 70, 125};
static const unsigned int menu_rows[GR_MENU_ROWS] = {30, 80};


/* ===== Private functions ===== */

static void menuDrawIconItem(int x, int y, const char *imgfile, char *text, int selected)
{
	struct romfs_file *file;
	int tx;
	void *font;
	
	/* 'Selected' box */
	if (selected) {
		grFillRect(x-1, y-1, x+GR_MENUITEM_WIDTH+1, y+GR_MENUITEM_HEIGHT+1, GR_COLOR_RED);
	}

	/* Image */
	if (imgfile) {
		file = romfs_find_file(imgfile);
		if (file) grImageOut(NULL, x, y, file->data);
	} else {
		grFillRect(x, y, x+GR_MENUITEM_WIDTH, y+GR_MENUITEM_HEIGHT, GR_COLOR_GRAY);
	}

	/* Text */
	font = grLoadFont(GR_MENUITEM_FONT);
	tx = x + (GR_MENUITEM_WIDTH  - grTextWidth(font, text)) / 2;
	grTextOut(NULL, font, tx, y+GR_MENUITEM_HEIGHT+1, GR_COLOR_BLACK, text);
}

static void menuDrawConfigItem(rect_t *rect, MenuItem *item, int selected, MenuHandler handler)
{
	void *font;
	char *value;
	int val_w, val_h;

	/* 'selected' box */
	if (selected) {
		grFillRect(rect->x, rect->y, rect->x + rect->w, rect->y + rect->h, GR_COLOR_RED);
		grFillRect(rect->x + 1, rect->y + 1, rect->x + rect->w - 1, rect->y + rect->h - 1, GR_COLOR_GRAY);
	}

	/* Text */
	font = grLoadFont(GR_MENUITEM_FONT);
	grTextOut(NULL, font, rect->x + 2, rect->y + 2, GR_COLOR_BLACK, item->name);

	/* Value */
	if (handler) {
		value = (char *)handler(MENU_GET_VALUE, item);
		if (value) {
			val_w = grTextWidth(font, value);
			val_h = grTextHeight(font);
			grTextOut(NULL, font, rect->x + rect->w - val_w - 2, rect->y + rect->h - val_h - 2,
				GR_COLOR_BLACK, value);
		}
	}
}

static void grDisplayMenu(Menu *menu)
{
	int i, col, row;
	rect_t rect;

	/* Clear screen */
	grFillRect(0, 20, 176, 132, GR_COLOR_WHITE);
	
	/* Icons-type menu */
	if (menu->type == MENU_TYPE_ICONS) {
		for (i = 0; i < menu->count; i++) {
			col = i % GR_MENU_COLS;
			row = i / GR_MENU_COLS;
			if (row >= GR_MENU_ROWS) break;

			menuDrawIconItem(menu_cols[col], menu_rows[row], menu->items[i].imgfile, menu->items[i].name,
					   menu->selected == i);
		}
	}

	/* Config-type menu */
	if (menu->type == MENU_TYPE_CONFIG) {
		rect.x = 20;
		rect.y = 27;
		rect.w = 150;
		rect.h = 25;
		for (i = 0; i < menu->count; i++) {
			/* Check screen boundaries */
			if ( (rect.y + rect.h) > 130 ) break;
			/* Draw item */
			menuDrawConfigItem(&rect, &menu->items[i], menu->selected == i, menu->handler);
			rect.y += rect.h;
		}
	}
}

static void menuMessageHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	Menu *menu = (Menu *)p;
	MenuItem *mi;
	int i;
	
	switch (msgCode) {
		case MSG_REDRAW:
			grDisplayMenu(menu);
			break;
			
		case MSG_KEY_PRESSED:
			if ( (msgParam >= '1') && (msgParam <= '9') ) {
				i = msgParam - '1';
				if (i < menu->count) {
					menu->selected = i;
					msgParam = 'R';
				}
			}
			/* Cancel */
			if (msgParam == 'C') {
				msgUnregisterWindow(window);
				if (menu->handler) menu->handler(MENU_EXIT, NULL);
				msgPostMessage(menu->parent, MSG_MENUCLICK, 0, NULL);
			}
			/* Return */
			if (msgParam == 'R') {
				mi = &menu->items[menu->selected];
				/* Call menu handler, if exist */
				if (menu->handler) {
					if (menu->handler(MENU_ITEM_CLICK, mi)) return;
				}
				if (mi->submenu) {
					menuShow(mi->submenu, window);
				} else if (mi->id) {
					msgUnregisterWindow(window);
					msgPostMessage(menu->parent, MSG_MENUCLICK, mi->id, NULL);
				}
			}
			/* Move left */
			if ( (msgParam == '*') && (menu->selected > 0) ) {
				menu->selected--;
				msgInvalidateWindow(window);
			}
			/* Move right */
			if ( (msgParam == '#') && (menu->selected < (menu->count-1)) ) {
				menu->selected++;
				msgInvalidateWindow(window);
			}
			break;

		/* Submenu click */
		case MSG_MENUCLICK:
			if (msgParam) {
				msgUnregisterWindow(window);
				msgPostMessage(menu->parent, MSG_MENUCLICK, msgParam, NULL);
			}
			break;
	}
}

/* ===== Exported functions ===== */

void menuShow(Menu *menu, void *parent)
{
	if (!menu) return;
	
	menu->parent = parent;
	menu->selected = 0;
	msgRegisterWindow("Menu", 0, menuMessageHandler, menu);
}
