
#ifndef _MALLOC_H
#define _MALLOC_H


typedef struct meminfo {
	unsigned int	ram_size;		/* Total RAM size */
	unsigned int	ram_free;		/* Free memory size */
	unsigned int	ram_data;		/* RAM data segment size */
	unsigned int	ram_bss;		/* RAM bss segment size */
} meminfo_t;

void memInit(void);

void *malloc(unsigned int size);
void free(void *p);
meminfo_t *meminfo(void);

#endif
