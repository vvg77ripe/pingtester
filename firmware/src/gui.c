
#include <config.h>
#include <stdio.h>
#include <string.h>

#include <drivers/ethernet.h>
#include <fs/romfs.h>
#include <grlib/grlib.h>
#include <grlib/menus.h>
#include <grlib/dialogs.h>
#include <net/ip.h>
#include <net/dhcp.h>

#include <os/messages.h>
#include <os/malloc.h>
#include <os/timer.h>
#include <registry.h>

#include "gui.h"


static const char circstr[4] = {'|', '/', '-', '\\'};
static unsigned char circpos = 0;
static char buf[50];
static char menubuf[50];

static unsigned int cntr_rx;
static unsigned int cntr_tx;

/* ===== Embedded applications ===== */

void shutdown(void);

void app_vct(void);
void app_ping(void);
void app_update(void);

/* ===== MENUS ===== */

#define ID_VCT			101
#define ID_PING			102

static const MenuItem itemsTests[] = {
	{ID_VCT, "Кабель", NULL},
	{ID_PING, "Ping", "ping.raw"}
};
static Menu menuTests = { itemsTests, sizeof(itemsTests) / sizeof(MenuItem), MENU_TYPE_ICONS };

#define ID_IP4_TYPE		201
#define ID_IP4_ADDRESS	202
#define ID_IP4_MASK		203
#define ID_IP4_GATEWAY	204
#define ID_IP4_DNS		205

/* === Config > IPv4 === */

static const MenuItem itemsIP4[] = {
	{ID_IP4_TYPE, "Тип адреса"},
	{ID_IP4_ADDRESS, "IP Адрес"},
	{ID_IP4_MASK, "Маска подсети"},
	{ID_IP4_GATEWAY, "Шлюз по умолчанию"},
	{ID_IP4_DNS, "Сервер DNS"}
};
static int handlerIP4(unsigned int code, MenuItem *item);
static Menu menuIP4 = { itemsIP4, sizeof(itemsIP4) / sizeof(MenuItem), MENU_TYPE_CONFIG, handlerIP4 };

/* === Config > Ethernet === */

#define ID_ETH_MAC		401
#define ID_ETH_SPEED	402

static const MenuItem itemsEthernet[] = {
	{ID_ETH_MAC, "MAC Адрес"},
	{ID_ETH_SPEED, "Скорость интерфейса"}
};
static int handlerEthernet(unsigned int code, MenuItem *item);
static Menu menuEthernet = { itemsEthernet, sizeof(itemsEthernet) / sizeof(MenuItem), MENU_TYPE_CONFIG, handlerEthernet };

/* === Config > USB === */

#define ID_USB_DEVICE	601
#define ID_USB_CARDMODE	602

static const MenuItem itemsUSB[] = {
	{ID_USB_DEVICE, "Устройство USB"},
	{ID_USB_CARDMODE, "Режим карты памяти"}
};
static int handlerUSB(unsigned int code, MenuItem *item);
static Menu menuUSB = { itemsUSB, sizeof(itemsUSB) / sizeof(MenuItem), MENU_TYPE_CONFIG, handlerUSB };

/* === Config > Tester === */

#define ID_TESTER_LIGHT	501

static const MenuItem itemsTester[] = {
	{ID_TESTER_LIGHT, "Время выключения подсветки"}
};
static int handlerTester(unsigned int code, MenuItem *item);
static Menu menuTester = { itemsTester, sizeof(itemsTester) / sizeof(MenuItem), MENU_TYPE_CONFIG, handlerTester };

/* === Config === */

static const MenuItem itemsConfig[] = {
	{0, "IPv4", NULL, &menuIP4},
	{0, "PPP", NULL, NULL},
	{0, "Ethernet", NULL, &menuEthernet},
	{0, "USB", "usb.raw", &menuUSB},
	{0, "Тестер", NULL, &menuTester}
};
static Menu menuConfig = { itemsConfig, sizeof(itemsConfig) / sizeof(MenuItem), MENU_TYPE_ICONS };

/* === Utils === */

#define ID_UTILS_UPDATE	301

static const MenuItem itemsUtils[] = {
	{ID_UTILS_UPDATE, "SW Update", NULL}
};
static Menu menuUtils = { itemsUtils, sizeof(itemsUtils) / sizeof(MenuItem), MENU_TYPE_ICONS };

/* === Main menu === */

static const MenuItem itemsMain[] = {
	{0, "Тесты", NULL, &menuTests},
	{0, "Настройки", "config.raw", &menuConfig},
	{0, "Утилиты", NULL, &menuUtils}
};
static Menu menuMain = { itemsMain, sizeof(itemsMain) / sizeof(MenuItem), MENU_TYPE_ICONS };


/* ===== Private functions ===== */

static char ip4_editbuf[50];

static int storeIP4(int type, char *buffer, void *p)
{
	unsigned char addr[4];

	if (type == DLG_OK) {
		/* Parse IP address */
		if (!inet_aton(addr, buffer)) return 0;

		/* Save IP address */
		regWriteValue((unsigned char)(unsigned int)p, addr, 4);
		return 1;
	}

	return 0;
}

static int handlerIP4(unsigned int code, MenuItem *item)
{
	unsigned char *v;

	if (code == MENU_GET_VALUE) {
		switch (item->id) {
			case ID_IP4_TYPE:
				return (int) "Статический IP";

			case ID_IP4_ADDRESS:
				v = regGetValue(SYS_REG_IP4_ADDRESS, NULL);
				if (v) sprintf(menubuf, "%u.%u.%u.%u", v[0], v[1], v[2], v[3]);
				return (int) menubuf;
			case ID_IP4_MASK:
				v = regGetValue(SYS_REG_IP4_MASK, NULL);
				if (v) sprintf(menubuf, "%u.%u.%u.%u", v[0], v[1], v[2], v[3]);
				return (int) menubuf;
			case ID_IP4_GATEWAY:
				v = regGetValue(SYS_REG_IP4_GATEWAY, NULL);
				if (v) sprintf(menubuf, "%u.%u.%u.%u", v[0], v[1], v[2], v[3]);
				return (int) menubuf;
			case ID_IP4_DNS:
				v = regGetValue(SYS_REG_IP4_DNS, NULL);
				if (v) sprintf(menubuf, "%u.%u.%u.%u", v[0], v[1], v[2], v[3]);
				return (int) menubuf;
		}
	}

	if (code == MENU_ITEM_CLICK) {
		switch (item->id) {
			case ID_IP4_ADDRESS:
				inet_ntoa(ip4_editbuf, regGetValue(SYS_REG_IP4_ADDRESS, NULL));
				dlgGetString("IP Адрес", ip4_editbuf, 20, storeIP4, (void *)SYS_REG_IP4_ADDRESS);
				break;
			case ID_IP4_MASK:
				inet_ntoa(ip4_editbuf, regGetValue(SYS_REG_IP4_MASK, NULL));
				dlgGetString("Маска подсети", ip4_editbuf, 20, storeIP4, (void *)SYS_REG_IP4_MASK);
				break;
			case ID_IP4_GATEWAY:
				inet_ntoa(ip4_editbuf, regGetValue(SYS_REG_IP4_GATEWAY, NULL));
				dlgGetString("Шлюз по умолчанию", ip4_editbuf, 20, storeIP4, (void *)SYS_REG_IP4_GATEWAY);
				break;
			case ID_IP4_DNS:
				inet_ntoa(ip4_editbuf, regGetValue(SYS_REG_IP4_DNS, NULL));
				dlgGetString("Сервер DNS", ip4_editbuf, 20, storeIP4, (void *)SYS_REG_IP4_DNS);
				break;
		}
		return 1;
	}

	if (code == MENU_EXIT) {
		ipUpdateConfig();
	}

	return 0;
}

static int handlerEthernet(unsigned int code, MenuItem *item)
{
	unsigned char *mac;

	if (code == MENU_GET_VALUE) {
		switch (item->id) {
			case ID_ETH_MAC:
				mac = ifGetAddress();
				sprintf(menubuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				return (int) menubuf;
			case ID_ETH_SPEED:
				return (int) "Авто";
		}
	}

	if (code == MENU_ITEM_CLICK) {
		return 1;
	}

	return 0;
}

static int handlerUSB(unsigned int code, MenuItem *item)
{
	if (code == MENU_GET_VALUE) {
		switch (item->id) {
			case ID_USB_DEVICE:
				return (int) "Card Reader";
			case ID_USB_CARDMODE:
				return (int) "Чтение/Запись";
		}
	}

	if (code == MENU_ITEM_CLICK) {
		return 1;
	}

	return 0;
}

static int handlerTester(unsigned int code, MenuItem *item)
{
	if (code == MENU_GET_VALUE) {
		switch (item->id) {
			case ID_TESTER_LIGHT:
				sprintf(menubuf, "%u сек", 15);
				return (int) menubuf;
		}
	}

	if (code == MENU_ITEM_CLICK) {
		return 1;
	}

	return 0;
}

static int guiSpeedItem(void *font, int x, int y, char *name, int allowed, int active)
{
	int fw, fh;

	/* Item width and height */
	fw = grTextWidth(font, name);
	fh = grTextHeight(font);

	/* Active item */
	if (active) {
		grFillRect(x, y, x+fw+1, y+fh, GR_COLOR_BLUE);
		grTextOut(NULL, font, x, y, GR_COLOR_WHITE, name);
		return fw;
	}

	grTextOut(NULL, font, x, y, allowed ? GR_COLOR_BLACK : GR_COLOR_GRAY, name);
	return fw;
}

/* guiMainScreen()
 *   Draw main screen
 */
static void guiMainScreen()
{
	void *font;
	int fw, fh;
	meminfo_t *mem;

	/* Clear screen */
	grFillRect(0, 20, 176, 132, GR_COLOR_WHITE);

	/* Greeting */
	font = grLoadFont(GR_FONT_NORMAL);
	grTextOut(NULL, font, 10, 40, GR_COLOR_BLACK, "Ethernet Ping Tester.");
	grTextOut(NULL, font, 10, 52, GR_COLOR_BLACK, "Модель PT-1E. Версия ПО " FIRMWARE_VERSION ".");

	mem = meminfo();
	sprintf(buf, "%u bytes total, %u bytes free", mem->ram_size, mem->ram_free);
	grTextOut(NULL, font, 10, 70, GR_COLOR_BLACK, buf);
	sprintf(buf, "%u bytes static, %u bytes bss", mem->ram_data, mem->ram_bss);
	grTextOut(NULL, font, 10, 80, GR_COLOR_BLACK, buf);

	/* Softkeys */
	font = grLoadFont(GR_FONT_BIG);
	fh = grTextHeight(font);
	grTextOut(NULL, font, 2, 132-fh, GR_COLOR_BLACK, "OFF");
	fw = grTextWidth(font, "Ping") + 2;
	grTextOut(NULL, font, 176-fw, 132-fh, GR_COLOR_BLACK, "Ping");
}

static void guiMessageHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	switch (msgCode) {
		case MSG_REDRAW:
			guiMainScreen();
			break;
			
		case MSG_KEY_PRESSED:
			if (msgParam == 'M') menuShow(&menuMain, window);
			if (msgParam == 'R') dhcpStart();
			break;

		case MSG_LONG_KEYPRESS:
			if (msgParam == 'C') shutdown();
			break;

		case MSG_MENUCLICK:
			if (msgParam == ID_VCT) app_vct();
			if (msgParam == ID_PING) app_ping();
			if (msgParam == ID_UTILS_UPDATE) app_update();
			break;

		case MSG_TIMER:
			ipTimers();
			guiStatusLine();
			break;
	}
}

/* ===== Exported functions ===== */

void guiStatusLine()
{
	void *font;
	int x;
	unsigned int ipad;
	struct romfs_file *file;
	unsigned short phy_status;
	unsigned char phy_speed;

	grFillRect(0, 0, 176, 19, GR_COLOR_WHITE);
	grFillRect(0, 19, 176, 20, GR_COLOR_GRAY);

	circpos++;
	if (circpos >= 4) circpos = 0;
	font = grLoadFont(GR_FONT_BIG);
	buf[0] = circstr[circpos]; buf[1] = 0;
	grTextOut(NULL, font, 150, 2, GR_COLOR_BLUE, buf);

	/* Ethernet interface speed */
	phy_status = EthPHYRead(17);
	phy_speed = 0;
	if (phy_status & PHY_REG17_LINK) {
		if (phy_status & PHY_REG17_100) {
			if (phy_status & PHY_REG17_FD) phy_speed = 4; else phy_speed = 3;
		} else {
			if (phy_status & PHY_REG17_FD) phy_speed = 2; else phy_speed = 1;
		}
	}
	font = grLoadFont(GR_FONT_SMALL);
	x = 2;
	x += guiSpeedItem(font, x, 2, "10HD", 1, phy_speed == 1);
	x += 3;
	x += guiSpeedItem(font, x, 2, "10FD", 1, phy_speed == 2);
	x += 3;
	x += guiSpeedItem(font, x, 2, "100HD", 1, phy_speed == 3);
	x += 3;
	x += guiSpeedItem(font, x, 2, "100FD", 1, phy_speed == 4);
	x += 3;

	if ( (phy_status & PHY_REG17_LINK) && (phy_status & PHY_REG17_MDIX) ) {
		grTextOut(NULL, font, x, 2, GR_COLOR_RED, "X");
	}

	/* Our IP address */
	ipad = ipGetAddress();
	sprintf(buf,  "%u.%u.%u.%u", (ipad >> 24) & 0xFF, (ipad >> 16) & 0xFF, (ipad >> 8) & 0xFF, ipad & 0xFF);
	grTextOut(NULL, font, 2, 10, GR_COLOR_BLACK, buf);

	/* RX and TX counters */
	sprintf(buf, "RX %u", cntr_rx);
	grTextOut(NULL, font, 100, 2, GR_COLOR_BLACK, buf);
	sprintf(buf, "TX %u", cntr_tx);
	grTextOut(NULL, font, 100, 10, GR_COLOR_BLACK, buf);

	/* Battery status */
	file = romfs_find_file("ac_25.raw");
	if (file) grImageOut(NULL, 169, 1, file->data);
}

void guiUpdateCounters(unsigned int rx, unsigned int tx)
{
	cntr_rx = rx;
	cntr_tx = tx;
}

void guiStart()
{
	void *wnd;

	guiStatusLine();

	/* Register main window */
	wnd = msgRegisterWindow("Main Window", 0, guiMessageHandler, NULL);

	/* Register update timer */
	tmrRegisterTimer(wnd, 500, 0, 1);
}
