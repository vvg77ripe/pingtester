// bin2efw.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../src/config.h"
#include <string.h>

struct fw_header {
	unsigned char	sign[4];		/* 'EFW!' */
	unsigned char	version[12];
	unsigned char	checksum[16];	/* MD5 */
};

static char buf[1024];
static char header[512];

static void ProcessFile(char *binfile, char *fwfile)
{
	struct fw_header *hdr;
	FILE *bf, *ff;
	int len;

	bf = fopen(binfile, "rb");
	if (!bf) {
		printf("Cannot open binary file.\n");
		return;
	}

	ff = fopen(fwfile, "w+b");
	if (!ff) {
		fclose(bf);
		printf("Cannot open destination file.\n");
		return;
	}

	/* Write file header */
	hdr = (struct fw_header *)header;
	memset(header, 0, 512);
	/* Sign */
	hdr->sign[0] = 'E';
	hdr->sign[1] = 'F';
	hdr->sign[2] = 'W';
	hdr->sign[3] = '!';
	/* FW Version */
	sprintf((char *)hdr->version, FIRMWARE_VERSION);

	fwrite(header, 1, 512, ff);

	/* Copy binary data */
	while (len = fread(buf, 1, sizeof(buf), bf)) {
		fwrite(buf, 1, len, ff);
	}

	fclose(bf);
	fclose(ff);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: bin2efw <binfile> <destfile>\r\n");
		return 0;
	}

	ProcessFile(argv[1], argv[2]);

	return 0;
}
