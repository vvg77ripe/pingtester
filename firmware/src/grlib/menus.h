
#ifndef _MENUS_H
#define _MENUS_H

#include <os/messages.h>


#define MENU_TYPE_ICONS		1
#define MENU_TYPE_LIST		2
#define MENU_TYPE_CONFIG	3


typedef struct menu Menu;

typedef struct menuitem {
	unsigned int	id;
	char *			name;
	char *			imgfile;
	Menu *			submenu;
} MenuItem;


#define MENU_GET_VALUE		1
#define MENU_ITEM_CLICK		2
#define MENU_EXIT			3

typedef int (*MenuHandler)(unsigned int code, MenuItem *item);

struct menu {
	const MenuItem *	items;
	unsigned char		count;
	unsigned char		type;
	MenuHandler			handler;
	unsigned char		selected;
	void *				parent;
};


void menuShow(Menu *menu, void *parent);

#endif
