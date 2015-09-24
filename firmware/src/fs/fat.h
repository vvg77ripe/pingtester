
#ifndef _FAT_H
#define _FAT_H

int fatMount(Volume *vol);
int fatGetFileSectors(Volume *vol, char *file, unsigned int *sectors, unsigned int maxsectors, unsigned int *filesize);

#endif
