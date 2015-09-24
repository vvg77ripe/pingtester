
#include <config.h>
#include <string.h>
#include <board.h>
#include <stdio.h>

#include <net/ip.h>
#include <os/messages.h>
#include <os/timer.h>
#include <grlib/grlib.h>
#include <grlib/dialogs.h>
#include <grlib/window.h>
#include <grlib/menus.h>


#define PING_COUNT		7

typedef struct {
	unsigned int		ipad;
	unsigned short		id;
	unsigned short		seq;
	unsigned int		timestamp;
	unsigned int		replytime;
	unsigned short		size;
} ping_t;

static ping_t pinglist[PING_COUNT];

static unsigned short	ping_id;
static unsigned short	ping_seq;
static unsigned int		ping_ip;
static unsigned int		ping_count;
static unsigned int		ping_sent;
static unsigned int		ping_received;

static char update_flag;
static char pause;
static char buf[50], editbuf[30];

#define ID_PING_COUNT		101
#define ID_PING_YLEVEL		102
#define ID_PING_RLEVEL		103

static const MenuItem itemsPing[] = {
	{ID_PING_COUNT, "Число запросов ping"},
	{ID_PING_YLEVEL, "Граница желтого"},
	{ID_PING_RLEVEL, "Граница красного"}
};
static int handlerPing(unsigned int code, MenuItem *item);
static Menu menuPing = { itemsPing, sizeof(itemsPing) / sizeof(MenuItem), MENU_TYPE_CONFIG, handlerPing };

static const char ping_data[] = "-=* PingTester Ping Data *=- :: -=* PingTester Ping Data *=-";

/* ===== Private functions ===== */

static void pingSendRequest(unsigned int ipad, unsigned short size)
{
	int i;
	unsigned char icmp[8];
	pktbuf hdr, data;
	unsigned short checksum;
	ip_frame_hdr ip;

	/* Find empty space in ping list */
	for (i = 0; i < PING_COUNT; i++) {
		if (pinglist[i].ipad == 0) break;
	}
	/* Table is full - delete old entries */
	if (i >= PING_COUNT) {
		for (i = 1; i < PING_COUNT; i++) pinglist[i-1] = pinglist[i];
		i = PING_COUNT - 1;
	}

	if (size == 0) size = sizeof(ping_data);

	/* Fill structure */
	pinglist[i].ipad = ipad;
	pinglist[i].id = ping_id;
	pinglist[i].seq = ++ping_seq;
	pinglist[i].size = size;
	pinglist[i].timestamp = AT91C_BASE_RTTC->RTTC_RTVR;
	pinglist[i].replytime = 0;

	/* ICMP header */
	icmp[0] = 8;
	icmp[1] = 0;
	icmp[2] = 0;
	icmp[3] = 0;
	icmp[4] = ping_id >> 8;
	icmp[5] = ping_id & 0xFF;
	icmp[6] = ping_seq >> 8;
	icmp[7] = ping_seq & 0xFF;

	/* ICMP checksum */
	checksum = ip_chksum(~ip_chksum(0xFFFF, (unsigned char *)ping_data, size), icmp, 8);
	icmp[2] = checksum >> 8;
	icmp[3] = checksum & 0xFF;

	/* IP header */
	ipFillHeader(&ip, ipGetAddress(), ipad, IP_PROTO_ICMP);

	/* Send Request */
	hdr.next = &data;
	hdr.data = icmp;
	hdr.len = 8;
	data.next = NULL;
	data.data = (unsigned char *)ping_data;
	data.len = size;
	if (!ipSendPacket(&ip, &hdr)) {
		pinglist[i].ipad = 0;
		return;
	}

	ping_sent++;
}

static void pingStart(unsigned int ipad)
{
	/* Clear old ping results */
	memset(pinglist, 0, sizeof(pinglist));

	ping_ip = ipad;
	ping_count = 2000;
	ping_id = AT91C_BASE_RTTC->RTTC_RTVR & 0xFFFF;
	ping_seq = 0;

	ping_sent = 0;
	ping_received = 0;
	pause = 0;
}

static int pingEditHandler(int type, char *buffer, void *p)
{
	unsigned int ip;

	if (type == DLG_OK) {
		/* Parse IP address */
		if (!inet_aton((unsigned char *)&ip, buffer)) return 0;

		pingStart(ntohl(ip));
		return 1;
	}

	return 0;
}

static int handlerPing(unsigned int code, MenuItem *item)
{
	if (code == MENU_GET_VALUE) {
		switch (item->id) {
			case ID_PING_COUNT:
				return (int) "2000";
			case ID_PING_YLEVEL:
				return (int) "15 мс";
			case ID_PING_RLEVEL:
				return (int) "40 мс";
		}
	}

	if (code == MENU_ITEM_CLICK) {
		return 1;
	}

	return 0;
}

static void icmpPingHandler(ip_frame_hdr *ip, unsigned char *packet, unsigned short size)
{
	int i;
	unsigned int ipad;
	unsigned short id, seq;

	/* Expect echo reply */
	if (size < 8) return;
	if (packet[0] != 0) return;

	ipad = ntohl(ip->source_addr);
	id = (packet[4] << 8) | packet[5];
	seq = (packet[6] << 8) | packet[7];

	for (i = 0; i < PING_COUNT; i++) {
		if ( (pinglist[i].ipad == ipad) && (pinglist[i].id == id) && (pinglist[i].seq == seq) ) {
			pinglist[i].replytime = AT91C_BASE_RTTC->RTTC_RTVR;
			ping_received++;
			update_flag = 1;
		}
	}
}

static void pingRedraw(void *window, rect_t *rect)
{
	void *font;
	int i;
	int x, y, time, color;
	unsigned int ip;

	if (!rect) return;

	/* Window frame */
	wndDrawWindowFrame(window);
	wndDrawSoftkey(window, SOFTKEY_LEFT, "Exit");
	wndDrawSoftkey(window, SOFTKEY_RIGHT, "Ping");

	font = grLoadFont(GR_FONT_SMALL);

	if (pause) grTextOut(rect, font, 10, 24, GR_COLOR_RED, "[PAUSED]");

	font = grLoadFont(GR_FONT_NORMAL);

	/* Ping info */
	grTextOut(rect, font, 10, 12, GR_COLOR_BLACK, "IP Адрес:");
	if (ping_ip) {
		ip = htonl(ping_ip);
		grTextOut(rect, font, 60, 12, GR_COLOR_BLUE, inet_ntoa(buf, (unsigned char *)&ip));
	} else {
		grTextOut(rect, font, 60, 12, GR_COLOR_BLUE, "Не задан");
	}

	/* Ping statistics */
	grTextOut(rect, font, 2, 35, GR_COLOR_BLACK, "Отпр:");
	sprintf(buf, "%u", ping_sent);
	grTextOut(rect, font, 2, 45, GR_COLOR_BLACK, buf);
	grTextOut(rect, font, 2, 60, GR_COLOR_BLACK, "Прин:");
	sprintf(buf, "%u", ping_received);
	grTextOut(rect, font, 2, 70, GR_COLOR_BLACK, buf);

	/* Ping times */
	for (i = 0; i < PING_COUNT; i++) {
		/* Start position */
		x = 40 + i * 20;
		y = 85;
		/* Ping time */
		if (pinglist[i].replytime) {
			/* Time */
			time = pinglist[i].replytime - pinglist[i].timestamp;
			sprintf(buf, "%u", time);
			grTextOut(rect, font, x, y + 2, GR_COLOR_BLACK, buf);
			/* Line */
			if (time > 50) time = 50;
			color = GR_COLOR_GREEN;
			if (time > 15) color = GR_COLOR_YELLOW;
			if (time > 40) color = GR_COLOR_RED;
			grFillRect(rect->x + x - 1, rect->y + y - time - 1, rect->x + x + 11, rect->y + y + 1, GR_COLOR_GRAY);
			grFillRect(rect->x + x, rect->y + y - time, rect->x + x + 10, rect->y + y, color);
		} else {
			grTextOut(rect, font, x, y + 2, GR_COLOR_RED, "--");
		}
	}
}

static void pingHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	switch (msgCode) {
		case MSG_INIT:
			/* Set icmp packet handler */
			icmpRegisterHandler(icmpPingHandler);
			/* Register timers */
			tmrRegisterTimer(window, 500, 0, 1);		/* Ping timer */
			tmrRegisterTimer(window, 100, 0, 2);		/* Update timer */
			break;

		case MSG_DESTROY:
			/* Destroy timers */
			tmrDestroyTimer(window, 1);
			tmrDestroyTimer(window, 2);
			/* Unregister icmp packet handler */
			icmpRegisterHandler(NULL);
			break;

		case MSG_REDRAW:
			pingRedraw(window, (rect_t *)msgPtr);
			break;

		case MSG_KEY_PRESSED:
			if (msgParam == 'C') msgUnregisterWindow(window);
			if (msgParam == 'M') menuShow(&menuPing, window);
			if (msgParam == 'R') {
				dlgGetString("Введите IP адрес", editbuf, 20, pingEditHandler, NULL);
			}
			if (msgParam == '0') {
				pause = 1 - pause;
				msgInvalidateWindow(window);
			}
			break;

		case MSG_TIMER:
			/* Send ping request if any */
			if ( (msgParam == 1) && ping_count && !pause) {
				ping_count--;
				pingSendRequest(ping_ip, 0);
			}
			/* Redraw window if updated */
			if ( (msgParam == 2) && update_flag) {
				update_flag = 0;
				msgInvalidateWindow(window);
			}
			break;
	}
}

void app_ping()
{
	/* Initial address */
	sprintf(editbuf, "172.21.96.1");
	ping_ip = 0;
	ping_count = 0;
	pause = 0;

	/* Create window */
	msgRegisterWindow("Тест связи (Ping)", 0, pingHandler, NULL);
}
