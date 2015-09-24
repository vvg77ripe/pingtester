
#ifndef _GRLIB_H
#define _GRLIB_H

#define GR_COLOR_TRANPARENT		255

#define GR_COLOR_BLACK			0x00
#define GR_COLOR_WHITE			0xFF
#define GR_COLOR_RED			0xF9
#define GR_COLOR_GREEN			0xFA
#define GR_COLOR_YELLOW			0xFB
#define GR_COLOR_BLUE			0xFC
#define GR_COLOR_GRAY			0xF7

#define GR_FONT_SMALL			"small_7px.fnt"
#define GR_FONT_NORMAL			"smallr_9px.fnt"
#define GR_FONT_BIG				"coure_13px.fnt"

struct font_header {
	unsigned short		dfVersion;
	unsigned int		dfSize;
	char				dfCopyright[60];
	unsigned short		dfType;
	unsigned short		dfPoints;
	unsigned short		dfVertRes;
	unsigned short		dfHorizRes;
	unsigned short		dfAscent;
	unsigned short		dfInternalLeading;
	unsigned short		dfExternalLeading;
	char				dfItalic;
	char				dfUnderline;
	char				dfStrikeOut;
	unsigned short		dfWeight;
	unsigned char		dfCharSet;
	unsigned short		dfPixWidht;
	unsigned short		dfPixHeight;
	unsigned char		dfPitchAndFamily;
	unsigned short		dfAvgWidht;
	unsigned short		dfMaxWidht;
	unsigned char		dfFirstChar;
	unsigned char		dfLastChar;
	unsigned char		dfDefaultChar;
	unsigned char		dfBreakChar;
	unsigned short		dfWidhtBytes;
	unsigned int		dfDevice;
	unsigned int		dfFace;
	unsigned int		dfBitsPointer;
	unsigned int		dfBitsOffset;
	char				dfReserved;
	/* -- Version 3 fields
	unsigned int		dfFlags;
	unsigned short		dfAspace;
	unsigned short		dfBspace;
	unsigned short		dfCspace;
	unsigned int		dfColorPointer;
	char				dfReserved1[16];
	*/
	unsigned short		dfCharTable[];
} PACKED;

typedef struct rect {
	unsigned short		x;
	unsigned short		y;
	unsigned short		w;
	unsigned short		h;
} rect_t;

typedef void (*grRedrawHandler)(void);

void grDisplayInit(unsigned char *buffer, int width, int height, unsigned short *palette, grRedrawHandler handler);
void grDisplayRedraw(void);

void grColorSet(unsigned char idx, unsigned short color);
void grFillScreen(unsigned char color);
void grFillRect(int fx, int fy, int tx, int ty, unsigned char color);
void grScrollScreen(int fy, int ty, int rows);

void * grLoadFont(char *name);
int grTextOut(rect_t *rect, void *font, unsigned short x, unsigned short y, unsigned char color, const char *text);
int grTextWidth(void *font, char *text);
int grTextHeight(void *font);

void grImageOut(rect_t *rect, unsigned short x, unsigned short y, unsigned char *image);

#endif
