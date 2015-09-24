
#include <config.h>
#include <string.h>

#include "malloc.h"


#define STACK_SIZE		4096


typedef struct memblock {
	unsigned char	type;
	unsigned char	res;
	unsigned short	size;
} block_t;

static meminfo_t minfo;
static void *next_free_block;
static void *last_block;

extern unsigned char _srelocate[];
extern unsigned char _erelocate[];
extern unsigned char _szero[];
extern unsigned char _ezero[];

/* ===== Exported functions ===== */

void memInit()
{
	minfo.ram_size = 65536;
	minfo.ram_data = (unsigned int) (_erelocate - _srelocate);
	minfo.ram_bss = (unsigned int) (_ezero - _szero);
	minfo.ram_free = minfo.ram_size - minfo.ram_data - minfo.ram_bss - STACK_SIZE;

	next_free_block = _ezero;
	last_block = _ezero + minfo.ram_free;
}

void *malloc(unsigned int size)
{
	unsigned short blocklen;
	void *p;

	if (size > 65536) return NULL;

	/* Blocks must be word-aligned */
	blocklen = (size + 3) & ~3;

	/* Check available memory */
	if ((next_free_block + blocklen) > last_block) return NULL;

	p = next_free_block;
	next_free_block += blocklen;

	return p;
}

void free(void *p)
{
}

meminfo_t *meminfo()
{
	return &minfo;
}
