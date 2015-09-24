
#include <config.h>
#include <string.h>
#include <stdio.h>

#include <drivers/ethernet.h>
#include <grlib/grlib.h>
#include <grlib/window.h>
#include <os/messages.h>
#include <os/timer.h>


#define VCT_STATE_IDLE		0
#define VCT_STATE_TESTING	1
#define VCT_STATE_FAIL		2
#define VCT_STATE_READY		3

static char state;
static unsigned short save_r0;
static unsigned short reg0;
static unsigned short reg1;
static char buf[50];


static void vctInit()
{
	/* Save Reg0, Force 100mbit */
	save_r0 = EthPHYRead(0);
	EthPHYWrite(0, 0xA100);

	/* Enable workaround */
	EthPHYWrite(29, 0x000C);
	EthPHYWrite(30, 0x4100);
	EthPHYWrite(29, 0x000F);
	EthPHYWrite(30, 0x9893);
}

static void vctDone()
{
	/* Undo workaround */
	EthPHYWrite(29, 0x000C);
	EthPHYWrite(30, 0x0000);
	EthPHYWrite(29, 0x000F);
	EthPHYWrite(30, 0x9899);

	/* Restore Reg0 */
	EthPHYWrite(0, 0x8000 | save_r0);
}

static void vctResult(char *str, unsigned short reg)
{
	char *pstate;
	unsigned int plen;
	int pamp;

	pstate = "";
	switch ( (reg >> 13) & 3 ) {
		case 0:
			pstate = "Отражение";
			break;
		case 1:
			pstate = "Замыкание";
			break;
		case 2:
			pstate = "Обрыв";
			break;
		case 3:
			sprintf(str, "Ошибка измерения");
			return;
	}

	if ((reg & 0xFF) == 0xFF) {
		sprintf(str, "Подключен к оборудованию");
		return;
	}

	pamp = (((reg >> 8) & 0x1F) - 0x13) * 100 / 12;
	plen = (reg & 0xFF) * 0.7861 - 18.862;

	sprintf(str, "%s на %uм (Отр %i%%)", pstate, plen, pamp);
}

static void vctRedraw(void *window, rect_t *rect)
{
	void *font;

	if (!rect) return;

	wndDrawWindowFrame(window);

	font = grLoadFont(GR_FONT_NORMAL);

	/* VCT Results */
	switch (state) {
		case VCT_STATE_IDLE:
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Нажмите 'СТАРТ'");
			wndDrawSoftkey(window, SOFTKEY_LEFT, "Exit");
			wndDrawSoftkey(window, SOFTKEY_RIGHT, "Start");
			break;

		case VCT_STATE_TESTING:
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Тестирование...");
			break;

		case VCT_STATE_READY:
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Готово");
			/* 1-2 */
			grTextOut(rect, font, 10, 35, GR_COLOR_BLACK, "1-2");
			vctResult(buf, reg1);
			grTextOut(rect, font, 30, 35, GR_COLOR_BLACK, buf);
			/* 3-6 */
			grTextOut(rect, font, 10, 50, GR_COLOR_BLACK, "3-6");
			vctResult(buf, reg0);
			grTextOut(rect, font, 30, 50, GR_COLOR_BLACK, buf);
			/* - */
			wndDrawSoftkey(window, SOFTKEY_LEFT, "Exit");
			wndDrawSoftkey(window, SOFTKEY_RIGHT, "Retry");
			break;
	}
}

static void vctHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	switch (msgCode) {
		case MSG_REDRAW:
			vctRedraw(window, (rect_t *)msgPtr);
			break;

		case MSG_KEY_PRESSED:
			if (msgParam == 'C') {
				vctDone();
				tmrDestroyTimer(window, 1);
				msgUnregisterWindow(window);
			}
			if (msgParam == 'R') {
				EthPHYWrite(26, 0x8000);
				state = VCT_STATE_TESTING;
				msgInvalidateWindow(window);
			}
			break;

		case MSG_TIMER:
			if (state == VCT_STATE_TESTING) {
				reg0 = EthPHYRead(26);
				reg1 = EthPHYRead(27);
				if ( !(reg0 & 0x8000) ) {
					state = VCT_STATE_READY;
					msgInvalidateWindow(window);
				}
			}
			break;
	}
}

void app_vct()
{
	void *wnd;

	state = VCT_STATE_IDLE;
	vctInit();

	/* Create window */
	wnd = msgRegisterWindow("Virtual Cable Tester", 0, vctHandler, NULL);
	if (!wnd) return;

	/* Create refresh timer */
	tmrRegisterTimer(wnd, 500, 0, 1);
}
