
#include <config.h>
#include <stdio.h>
#include <string.h>

#include "grlib/grlib.h"
#include "fs/romfs.h"


static unsigned char *screen;
static rect_t screenrect;
static int screen_w;
static int screen_h;
static unsigned short *pal;
static grRedrawHandler screen_handler;
static unsigned char redraw_flag;

/* Compiled-in windows palette */
extern unsigned char winpalette[];


/* ===== Exported functions ===== */

void grDisplayInit(unsigned char *buffer, int width, int height, unsigned short *palette, grRedrawHandler handler)
{
	int i;
	unsigned char *p;
	unsigned char r, g, b;

	/* Save screen pointers */
	screen = buffer;
	screen_w = width;
	screen_h = height;
	pal = palette;
	screen_handler = handler;

	screenrect.x = 0;
	screenrect.y = 0;
	screenrect.w = width;
	screenrect.h = height;

	/* Load default palette */
	p = winpalette;
	for (i = 0; i < 256; i++) {
		b = *p++;
		g = *p++;
		r = *p++;
		p++;

		pal[i] = (((r >> 3) & 0x1F ) << 11) | (((g >> 2) & 0x3F) << 5) | (b & 0x1F);
	}
}

void grDisplayRedraw()
{
	if (!redraw_flag) return;
	redraw_flag = 0;
	if (screen_handler) screen_handler();
}

/* ===== Basic display functions ===== */

signed int fputc(signed int c, FILE *pStream)
{
	return c;
}

signed int fputs(const char *pStr, FILE *pStream)
{
	void * font;
	int h;

	font = grLoadFont(GR_FONT_NORMAL);
	h = grTextHeight(font);
	grScrollScreen(20, 115, h);
	grFillRect(0, 115 - h, screen_w, 115, GR_COLOR_WHITE);
	grTextOut(NULL, font, 0, 115 - h, GR_COLOR_BLACK, pStr);
	return 0;
}

void grColorSet(unsigned char idx, unsigned short color)
{
	pal[idx] = color;
}

void grFillScreen(unsigned char color)
{
	memset(screen, color, screen_w * screen_h);
	redraw_flag = 1;
}

void FASTCODE grFillRect(int fx, int fy, int tx, int ty, unsigned char color)
{
	unsigned char *buf;
	int lx, ly, sx;

	/* Calculate start position */
	buf = &screen[fx + fy * screen_w];
	/* Horisontal and vertical sizes */
	lx = tx - fx;
	ly = ty - fy;
	/* Horizontal skip */
	sx = screen_w - lx;

	if ((lx <= 0) || (ly <= 0)) return;

	/* Fill the rect */
	while (ly--) {
		lx = tx - fx;
		while (lx--) *(buf++) = color;
		buf += sx;
	}

	redraw_flag = 1;
}

void FASTCODE grScrollScreen(int fy, int ty, int rows)
{
	unsigned char *from, *to;
	int i, size;

	to = &screen[fy * screen_w];
	from = &screen[(fy+rows) * screen_w];
	size = (ty - fy - rows) * screen_w;

	for (i = 0; i < size; i++) *to++ = *from++;

	redraw_flag = 1;
}

/* ===== Font supporting functions ===== */

void * grLoadFont(char *name)
{
	struct romfs_file *fontfile;

	fontfile = romfs_find_file(name);
	if (!fontfile) return NULL;

	return fontfile->data;
}

int FASTCODE grTextOut(rect_t *rect, void *font, unsigned short x, unsigned short y,
					   unsigned char color, const char *text)
{
	int i, j, charw;
	int posx, posy;
	unsigned char ch;
	struct font_header *header = (struct font_header *) font;
	unsigned char *bits, *out;

	/* Fonts version 2.0 is only supported */
	if (header->dfVersion != 0x0200) return 0;

	/* Adjust positions */
	if (!rect) rect = &screenrect;
	x += rect->x;
	y += rect->y;

	/* Set initial position */
	posx = x;

	for (i = 0; i < strlen(text); i++) {
		/* Get a character from string */
		ch = text[i];
		if (ch < 32) continue;

		if ( (ch < header->dfFirstChar) || (ch > header->dfLastChar) ) {
			ch = header->dfDefaultChar;
		} else {
			ch = ch - header->dfFirstChar;
		}

		/* Get character width and bitmap pointer */
		charw = header->dfCharTable[ch * 2];
		bits = (unsigned char *) font + header->dfCharTable[ch * 2 + 1];

		/* Check screen boundaries */
		if ((posx + charw) >= screen_w) break;

		/* Write character  */
		while (charw > 0) {
			/* Write one column */
			for (posy = y; posy < (y + header->dfPixHeight); posy++) {
				/* Check screen boundaries */
				if (posy >= screen_h) break;

				/* Start position of scan line */
				out = &screen[posy * screen_w + posx];
	
				for (j = 0; j < 8; j++) {
					if (bits[0] & (0x80 >> j)) *out = color;
					out++;
				}

				/* Move to next byte */
				bits++;
			}

			/* Move to next column */
			if (charw >= 8) {
				posx += 8;
				charw -= 8;
			} else {
				posx += charw;
				charw = 0;
			}
		}

	}

	redraw_flag = 1;
	return posx - x;
}

int grTextWidth(void *font, char *text)
{
	int i, w;
	unsigned char ch;
	struct font_header *header = (struct font_header *) font;

	/* Fonts version 2.0 is only supported */
	if (header->dfVersion != 0x0200) return 0;

	w = 0;
	for (i = 0; i < strlen(text); i++) {
		/* Get a character from string */
		ch = text[i];
		if (ch < 32) continue;

		if ( (ch < header->dfFirstChar) || (ch > header->dfLastChar) ) {
			ch = header->dfDefaultChar;
		} else {
			ch = ch - header->dfFirstChar;
		}

		/* Get character width */
		w += header->dfCharTable[ch * 2];
	}

	return w;
}

int grTextHeight(void *font)
{
	struct font_header *header = (struct font_header *) font;
	return header->dfPixHeight;
}

/* ===== Image functions ===== */

void FASTCODE grImageOut(rect_t *rect, unsigned short x, unsigned short y, unsigned char *image)
{
	unsigned char *buf;
	unsigned short w, h, lx, sx;

	/* Adjust positions */
	if (!rect) rect = &screenrect;
	x += rect->x;
	y += rect->y;

	/* Horisontal and vertical sizes */
	w = *image++;
	h = *image++;
	/* Calculate start position */
	buf = &screen[x + (y+h-1) * screen_w];
	/* Horizontal skip */
	sx = screen_w + w;

	if ((w <= 0) || (h <= 0)) return;

	/* Fill the rect */
	while (h--) {
		lx = w;
		while (lx--) *buf++ = *image++;;
		buf -= sx;
	}

	redraw_flag = 1;
}
