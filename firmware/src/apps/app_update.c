
#include <config.h>
#include <string.h>
#include <stdio.h>
#include <board.h>

#include <os/messages.h>
#include <drivers/sdcard.h>
#include <fs/volume.h>
#include <grlib/grlib.h>
#include <grlib/window.h>
#include <usb/usbdevices.h>


#define UPD_STATE_SEARCHING		1
#define UPD_STATE_VERIFYING		2
#define UPD_STATE_READY			3
#define UPD_STATE_NOTFOUND		4
#define UPD_STATE_BADFILE		5

struct fw_header {
	unsigned char	sign[4];		/* 'EFW!' */
	unsigned char	version[12];
	unsigned char	checksum[16];	/* MD5 */
} PACKED;

static Volume *vol;
static char file_name[50];
static unsigned int file_sectors[20];
static int file_chunks;
static unsigned int file_size;
static struct fw_header file_header;
static char state;

static char buf[50];


static void FASTCODE do_update(unsigned char *buf, unsigned int *sectors, unsigned int count)
{
	unsigned int cn, sn, pn;
	unsigned int *flash, *ram;
	int i;

	/* Configure flash clock */
	AT91C_BASE_MC->MC_FMR |= (BOARD_MCK / 666666) << 16;

	/* Skip firmware header */
	(*sectors)++;
	(*(sectors+1))--;

	/* First page number */
	pn = 0;
	flash = (unsigned int *)AT91C_IFLASH;

	for (cn = 0; cn < count; cn++) {
		for (sn = 0; sn < *(sectors+1); sn++) {
			/* Read next sector */
			sdcBlockRead(buf, sn + *sectors, 1);
			ram = (unsigned int *)buf;

			/* Write first page */
			for (i = 0; i < 64; i++) *flash++ = *ram++;
			AT91C_BASE_MC->MC_FCR = (0x5A << 24) | (pn << 8) | AT91C_MC_FCMD_START_PROG;
			while ( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );
			pn++;

			/* Write next page */
			for (i = 0; i < 64; i++) *flash++ = *ram++;
			AT91C_BASE_MC->MC_FCR = (0x5A << 24) | (pn << 8) | AT91C_MC_FCMD_START_PROG;
			while ( !(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) );
			pn++;
		}

		/* Move to next chunk */
		sectors += 2;
	}

	/* Reset system */
	AT91C_BASE_RSTC->RSTC_RCR = (0xA5 << 24) | 0x05;
	while (1);
}

static void prepare_update()
{
	void *font;

	/* Disable USB */
	usbStop();

	/* Disable all interrupts */
	AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF;

	/* Display warning screen */
	grFillScreen(GR_COLOR_WHITE);
	font = grLoadFont(GR_FONT_NORMAL);
	grTextOut(NULL, font, 20, 20, GR_COLOR_RED, "Производится обновление ПО");
	grDisplayRedraw();
}

static void updRedraw(void *window, rect_t *rect)
{
	void *font;

	if (!rect) return;

	wndDrawWindowFrame(window);

	font = grLoadFont(GR_FONT_NORMAL);

	switch (state) {
		case UPD_STATE_SEARCHING:
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Поиск новой версии ПО...");
			break;
		case UPD_STATE_NOTFOUND:
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Файл ПО не найден.");
			wndDrawSoftkey(window, SOFTKEY_LEFT, "Exit");
			break;
		case UPD_STATE_READY:
			/* FW file name */
			grTextOut(rect, font, 10, 20, GR_COLOR_BLACK, "Файл ПО:");
			grTextOut(rect, font, 90, 20, GR_COLOR_BLUE, file_name);
			/* FW version */
			grTextOut(rect, font, 10, 32, GR_COLOR_BLACK, "Версия ПО:");
			grTextOut(rect, font, 90, 32, GR_COLOR_BLUE, (char *)file_header.version);
			/* FW file size */
			grTextOut(rect, font, 10, 44, GR_COLOR_BLACK, "Размер файла:");
			sprintf(buf, "%u Б", file_size);
			grTextOut(rect, font, 90, 44, GR_COLOR_BLUE, buf);

			grTextOut(rect, font, 10, 70, GR_COLOR_BLACK, "Нажмите 'СТАРТ'");
			wndDrawSoftkey(window, SOFTKEY_LEFT, "Exit");
			wndDrawSoftkey(window, SOFTKEY_RIGHT, "Start");
			break;
	}
}

static FASTCODE void updHandler(void *window, unsigned short msgCode, unsigned short msgParam, void *msgPtr, void *p)
{
	switch (msgCode) {
		case MSG_REDRAW:
			updRedraw(window, (rect_t *)msgPtr);
			break;

		case MSG_KEY_PRESSED:
			if (msgParam == 'C') {
				msgUnregisterWindow(window);
			}
			if (msgParam == 'R') {
				msgInvalidateWindow(window);
				if (state == UPD_STATE_READY) {
					prepare_update();
					do_update(vol->sector, file_sectors, file_chunks);
				}
			}
			break;
	}
}

static int updVerify()
{
	/* Get file sectors */
	file_chunks = volGetFileSectors(vol, file_name, file_sectors, 10, &file_size);
	if (!file_chunks) {
		state = UPD_STATE_NOTFOUND;
		return 0;
	}

	/* Read file header */
	if (!sdcBlockRead(vol->sector, *file_sectors, 1)) {
		state = UPD_STATE_NOTFOUND;
		return 0;
	}

	memcpy(&file_header, vol->sector, sizeof(file_header));

	/* Check header signature */
	if ( (file_header.sign[0] != 'E') || (file_header.sign[1] != 'F') ||
		  (file_header.sign[2] != 'W') || (file_header.sign[3] != '!') ) {
		state = UPD_STATE_NOTFOUND;
		return 0;
	}

	state = UPD_STATE_READY;
	return 1;
}

void app_update()
{
	void *wnd;

	/* Create window */
	state = UPD_STATE_SEARCHING;
	wnd = msgRegisterWindow("Software Update", 0, updHandler, NULL);
	if (!wnd) return;

	/* Mount sd card */
	vol = volGetVolume(VOL_SDCARD);
	if (!volMount(vol, sdcGetMedia())) {
		state = UPD_STATE_NOTFOUND;
		return;
	}

	sprintf(file_name, "pt-1.efw");
	updVerify();

	msgInvalidateWindow(wnd);
}
