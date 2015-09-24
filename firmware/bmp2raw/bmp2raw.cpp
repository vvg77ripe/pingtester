
#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include <stdio.h>
#include <string.h>


#define MAX_DATA_SIZE		10240

#define WORD unsigned short
#define DWORD unsigned int
#define LONG unsigned int

typedef struct tagBITMAPINFOHEADER {
  DWORD  biSize; 
  LONG   biWidth; 
  LONG   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount; 
  DWORD  biCompression; 
  DWORD  biSizeImage; 
  LONG   biXPelsPerMeter; 
  LONG   biYPelsPerMeter; 
  DWORD  biClrUsed; 
  DWORD  biClrImportant; 
} BITMAPINFOHEADER, *PBITMAPINFOHEADER; 

static unsigned char imgdata[MAX_DATA_SIZE];

static void ProcessFile(char *srcfile, char *destfile)
{
	int i;
	FILE *sf, *df;
	unsigned char rawhdr[2];
	unsigned int boffset, bsize;
	BITMAPINFOHEADER bhdr;

	sf = fopen(srcfile, "rb");
	if (!sf) {
		printf("Could not open '%s'.\r\n", srcfile);
		return;
	}

	/* Read bitmap header */
	fseek(sf, 10, SEEK_SET);
	fread(&boffset, 4, 1, sf);
	fread(&bhdr, sizeof(bhdr), 1, sf);

	/* Open destination file */
	df = fopen(destfile, "w+b");
	if (!df) {
		printf("Cannot open destination file '%s'.\r\n", destfile);
		fclose(sf);
		return;
	}

	/* Write raw header */
	rawhdr[0] = bhdr.biWidth;
	rawhdr[1] = bhdr.biHeight;
	fwrite(rawhdr, 2, 1, df);

	bsize = (bhdr.biWidth + 3) & 0xFFFC;
	for (i = 0; i < bhdr.biHeight; i++) {
		/* Read bitmap line */
		fseek(sf, boffset + i * bsize, SEEK_SET);
		fread(imgdata, 1, bsize, sf);

		/* Write raw data */
		fwrite(imgdata, bhdr.biWidth, 1, df);
	}

	fclose(sf);
	fclose(df);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: bmp2raw <srcfile> <destfile>\r\n");
		return 0;
	}

	ProcessFile(argv[1], argv[2]);

	return 0;
}
