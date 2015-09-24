
#ifndef _VOLUME_H
#define _VOLUME_H

#include <memories/Media.h>

/* Pre-defined volumes */
#define VOL_SDCARD			0

#define SECTOR_SIZE			512

#define PTYPE_FAT12			0x01
#define PTYPE_FAT16			0x06
#define PTYPE_FAT32			0x0B

typedef struct volume {
	Media *			media;
	unsigned char	ptype;
	/* Filesystem data */
	union {
		struct {
			unsigned char	bits;
			unsigned short	clustersize;
			unsigned int	fatstart;		/* First FAT sector number */
			unsigned int	rootstart;		/* First root dir sector number */
			unsigned int	datastart;		/* Cluster #2 sector number */
		} fat;
	};
	/* Cache */
	unsigned int	sectoroffset;
	unsigned int	sectornum;
	unsigned char	sector[SECTOR_SIZE];
} Volume;

Volume *volGetVolume(unsigned int index);
int volMount(Volume *vol, Media *media);
void volUnmount(Volume *vol);
int volReadSector(Volume *vol, unsigned int num);

int volGetFileSectors(Volume *vol, char *file, unsigned int *sectors, unsigned int maxsectors, unsigned int *filesize);

#endif
