
#include <config.h>
#include <string.h>

#include "volume.h"
#include "fat.h"


#define MAX_VOLUMES		1

static Volume volumes[MAX_VOLUMES];

/* Partition table record */
struct ptrecord {
	unsigned char	active;
	unsigned char	s_head;
	unsigned char	s_sector;
	unsigned char	s_cylinder;
	unsigned char	ptype;
	unsigned char	e_head;
	unsigned char	e_sector;
	unsigned char	e_cylinder;
	unsigned int	start;
	unsigned int	size;
} PACKED;

/* ===== Exported functions ===== */

Volume *volGetVolume(unsigned int index)
{
	return volumes;
}

/* volMount()
 *   Mounts specified volume
 */
int volMount(Volume *vol, Media *media)
{
	struct ptrecord *ptable;
	unsigned int scount;
	int r;

	/* Check arguments */
	if (!vol) return 0;
	if (!media) return 0;

	vol->media = media;

	/* Read the MBR */
	vol->sectoroffset = 0;
	vol->sectornum = 0xFFFFFFFF;
	if (!volReadSector(vol, 0)) return 0;

	/* Find valid partition table */
	do {
		/* Check MBR signature */
		if ( (vol->sector[510] != 0x55) || (vol->sector[511] != 0xAA) ) break;

		/* Partition table pointer */
		ptable = (struct ptrecord *)&vol->sector[0x1BE];
		if (ptable[0].ptype == 0) break;

		/* Check size bounds of this partition */
		scount = vol->media->size < 1;
		if (!ptable[0].start || !ptable[0].size) break;
		if (ptable[0].start > scount) break;
		if ( (ptable[0].start + ptable[0].size) > scount) break;

		/* Partition table entry looks valid -- trying to mount it */
		vol->sectoroffset = ptable[0].start;
		vol->sectornum = 0xFFFFFFFF;

		r = 0;
		switch (ptable[0].ptype) {
			case PTYPE_FAT16:
			case PTYPE_FAT32:
				r = fatMount(vol);
				break;
		}

		/* Partition mounted - exit now */
		if (r) return r;

	} while (0);

	/* There is no partition table or mounting error -- try auto-detect */

	/* Clear cache */
	vol->sectoroffset = 0;
	vol->sectornum = 0xFFFFFFFF;

	/* Only FAT is supported yet */
	return fatMount(vol);
}

void volUnmount(Volume *vol)
{
}

int volReadSector(Volume *vol, unsigned int num)
{
	if (!vol) return 0;
	if (!vol->media) return 0;

	/* Return cached data, if any */
	if (vol->sectornum == num) return 1;

	/* Read one sector */
	vol->sectornum = 0xFFFFFFFF;
	if (vol->media->read(vol->media, num, vol->sector + vol->sectoroffset, 1, NULL, NULL))
		return 0;

	/* Save cached sector number */
	vol->sectornum = num;

	return 1;
}

/* volGetFileSectors()
 *   Return physical file location as <start sector>-<sector count> pairs.
 *
 *   This function return number of start-count pairs written in *sectors, or 0
 * if file not found or other errors.
 */
int volGetFileSectors(Volume *vol, char *file, unsigned int *sectors, unsigned int maxsectors, unsigned int *filesize)
{
	return fatGetFileSectors(vol, file, sectors, maxsectors, filesize);
}
