
#include <config.h>
#include <string.h>

#include <drivers/eeprom.h>
#include <os/malloc.h>

#include "registry.h"


struct eeheader {
	unsigned char	sign[2];	/* 'CF' */
	unsigned short	size;		/* Size, in bytes, of the following configuration */
} PACKED;

static unsigned char defaults[] = {
	/* Model name */
	SYS_REG_MODEL_NAME, 6,
	'P', 'T', '-', '1', 'E', 0,
	/* Default system MAC address */
	SYS_REG_MAC_ADDRESS, 6,
	0x00, 0x52, 0x42, 0x03, 0x00, 0x00,
	/* IP configuration */
	SYS_REG_IP4_TYPE, 1,
	0,
	SYS_REG_IP4_ADDRESS, 4,
	172, 21, 96, 99,
	SYS_REG_IP4_MASK, 4,
	255, 255, 255, 0,
	SYS_REG_IP4_GATEWAY, 4,
	172, 21, 96, 1,
	/* USB configuration (default is cardreader) */
	SYS_REG_USB_DEVICE, 1,
	1,
	/* EOF */
	0
};
static unsigned char *registry = defaults;

/* ===== Exported functions ===== */

void regInit()
{
	struct eeheader header;
	unsigned char *data;
	int r;

	/* read nvram header */
	r = eepromBlockRead((unsigned char *)&header, 0, sizeof(header));
	if (r != sizeof(header)) return;

	/* Check signature */
	if ( (header.sign[0] != 'C') || (header.sign[1] != 'F') ) return;

	/* Allocate memory for config data */
	data = (unsigned char *)malloc(header.size + 128);
	if (!data) return;

	/* Read configuration */
	r = eepromBlockRead(data, sizeof(header), header.size);
	if (r != header.size) return;

	/* All OK */
	registry = data;
}

void regSave()
{
	struct eeheader header;
	unsigned char *r;
	unsigned char rlen;

	/* Calculate registry size */
	r = registry;
	header.size = 0;
	while ( (*r++) ) {
		rlen = *r++;
		r += rlen;
	}

	/* Prepare header */
	header.sign[0] = 'C';
	header.sign[1] = 'F';
	header.size = r - registry;

	/* Write header and data */
	eepromBlockWrite((unsigned char *)&header, 0, sizeof(header));
	eepromBlockWrite(registry, sizeof(header), header.size);
}

unsigned char *regGetValue(unsigned char key, unsigned char *out)
{
	unsigned char *r;
	unsigned char rkey, rlen;

	r = registry;

	/* Get next key */
	while ( (rkey = *r++) ) {
		/* Value length */
		rlen = *r++;
		if (rkey == key) {
			/* Copy to destination pointer, if given */
			if (out) memcpy(out, r, rlen);
			return r;
		}
		/* Skip value */
		r += rlen;
	}

	return NULL;
}

int regWriteValue(unsigned char key, unsigned char *value, unsigned char len)
{
	unsigned char *r;
	unsigned char rkey, rlen;

	r = registry;

	/* Get next key */
	while ( (rkey = *r++) ) {
		/* Value length */
		rlen = *r++;
		if (rkey == key) {
			/* Write value */
			if (len <= rlen) {
				memcpy(r, value, len);
				return 1;
			}
		}
		/* Skip value */
		r += rlen;
	}

	return 0;
}
