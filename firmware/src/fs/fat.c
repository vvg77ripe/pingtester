
#include <config.h>
#include <string.h>

#include "volume.h"
#include "fat.h"


struct fat16br {
	unsigned char		jump[3];
	char				oemname[8];
	unsigned short		sectorsize;		/* Bytes per sector */
	unsigned char		clustersize;	/* Sectors in cluster */
	unsigned short		rsectors;
	unsigned char		fatcopies;
	unsigned short		maxroot;
	unsigned short		sectorcount;	/* Number of Sectors in Partition Smaller than 32MB */
	unsigned char		media;
	unsigned short		fatsize;		/* Sectors per FAT */
	unsigned short		tracksize;		/* Sectors per track */
	unsigned short		heads;
	unsigned int		hidden;			/* Number of hidden sectors */
	unsigned int		sectors;		/* Number of sectors */
	unsigned short		ldn;			/* Logical Drive Number */
	unsigned char		extsign;		/* Extended signature */
	unsigned int		serial;
	unsigned char		volname[11];
	unsigned char		fatname[8];
} PACKED;

struct fatfile {
	unsigned char		name[8];
	unsigned char		ext[3];
	unsigned char		attr;
	unsigned char		ntattr;
	unsigned char		create_msec;
	unsigned short		create_time;
	unsigned short		create_date;
	unsigned short		accessdate;
	unsigned short		hicluster;
	unsigned short		time;
	unsigned short		date;
	unsigned short		cluster;
	unsigned int		size;
} PACKED;


/* ===== Private functions ===== */

static unsigned int fatNextSector(Volume *vol, unsigned int sector)
{
	/* If we are in root directory, stop in the last sector */
	if ( sector == (vol->fat.datastart-1) ) return 0;
	if ( sector < vol->fat.datastart) return sector + 1;

	/* If we are not crossing cluster bounds, return the next sector */
	if ( ((sector - vol->fat.datastart) % vol->fat.clustersize) != (vol->fat.clustersize - 1))
		return sector + 1;

	/* Else we must read the fat table and lookup the next cluster */
	/* But this is not implemented yet */

	return sector + 1;
}

static struct fatfile *fatFindRecord(Volume *vol, unsigned int sector, char *file)
{
	int i;
	unsigned char c;
	unsigned int sn;
	struct fatfile *ff;
	unsigned char name83[11];

	/* Convert file name to 8.3 format */
	memset(name83, 0x20, 11);
	/* Name */
	for (i = 0; i < 8; i++) {
		c = *(unsigned char *)file++;
		if (c == '.') break;
		if ( (c < 0x20) || (c == '/') ) {
			file--;
			break;
		}

		/* Uppercase letters */
		if ( (c >= 'a') && (c <= 'z') ) c -= 0x20;

		name83[i] = c;
	}
	/* Extension */
	for (i = 8; i < 11; i++) {
		c = *(unsigned char *)file++;
		if ( (c < 0x20) || (c == '/') ) break;

		/* Uppercase letters */
		if ( (c >= 'a') && (c <= 'z') ) c -= 0x20;

		name83[i] = c;
	}

	sn = sector;
	do {
		/* Read directory sector */
		if (!volReadSector(vol, sn)) return NULL;
		ff = (struct fatfile *)vol->sector;

		/* Find our file in the sector */
		for (i = 0; i < (SECTOR_SIZE / sizeof(struct fatfile)); i++) {
			/* If old 8.3 file name is matched, return record pointer */
			if (!memcmp(name83, ff[i].name, 11)) return &ff[i];
		}
	} while ( (sn = fatNextSector(vol, sn)) );

	return NULL;
}

static struct fatfile *fatFindFile(Volume *vol, char *file)
{
	return fatFindRecord(vol, vol->fat.rootstart, file);
}

/* ===== Exported functions ===== */

int fatMount(Volume *vol)
{
	struct fat16br *fat16rec;

	/* Read boot record */
	if (!volReadSector(vol, 0)) return 0;

	fat16rec = (struct fat16br *)vol->sector;

	/* FAT16 */
	if (!memcmp(fat16rec->fatname, "FAT16   ", 8)) {
		vol->fat.bits = 16;
		vol->fat.clustersize = fat16rec->clustersize;
		vol->fat.fatstart = fat16rec->rsectors;
		vol->fat.rootstart = vol->fat.fatstart + fat16rec->fatsize * fat16rec->fatcopies;
		vol->fat.datastart = vol->fat.rootstart + (fat16rec->maxroot * 32) / SECTOR_SIZE;

		return 1;
	}

	return 0;
}

int fatGetFileSectors(Volume *vol, char *file, unsigned int *sectors, unsigned int maxsectors,
					  unsigned int *filesize)
{
	struct fatfile *ff;
	unsigned int fs, sn, sc;

	/* Check arguments */
	if (!vol) return 0;

	/* Get directory entry for specified file */
	ff = fatFindFile(vol, file);
	if (!ff) return 0;

	/* Get file's first sector number */
	if (!ff->cluster) return 0;
	sn = vol->fat.datastart + (ff->cluster - 2) * vol->fat.clustersize;

	/* Store file size */
	if (filesize) *filesize = ff->size;
	fs = ff->size;

	/* Store sector-count pairs */
	sc = 0;
	if (sectors && maxsectors) {
		*sectors = sn + vol->sectoroffset;
		*(sectors+1) = 1;
		sc = 1;

		while ( (sn = fatNextSector(vol, sn)) ) {
			if (fs < SECTOR_SIZE) break;
			fs -= SECTOR_SIZE;

			*(sectors+1) += 1;
		}
	}

	return sc;
}
