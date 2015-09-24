
#ifndef _CONFIG_H
#define _CONFIG_H

#define FIRMWARE_VERSION	"0.1.0"

#define uint8	unsigned char
#define uint16	unsigned short
#define uint32	unsigned int

#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif

#define FASTCODE __attribute__ ((section (".ramfunc")))

#endif
