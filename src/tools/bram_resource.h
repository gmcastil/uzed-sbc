#ifndef BRAM_CTRL_H
#define BRAM_CTRL_H

#include <stdint.h>

struct bram_resource {
	/* User is responsible for setting the UIO device and map numbers */
	int uio_number;
	int map_number;
	/* Path to the node created in /dev */
	char *dev_path;
	/* Path to memory map in /sys */
	char *map_path;
	/* Device major and minor numbers */
	int dev_major;
	int dev_minor;
	/* Physical address of the block RAM */
	uint32_t map_addr;
	/* String identifier for the mapping */
	char *map_name;
	/* 
	 * Offset in bytes that has to be added to mmap() return to get to the
	 * actual device memory. This is usually zero.
	 */
	uint32_t map_offset;
	/* Size in bytes of the memory */
	uint32_t map_size;
};

int bram_summary(struct bram_resource *bram);
int bram_create(struct bram_resource *bram, int uio_number, int map_number);
int bram_destroy(struct bram_resource *bram);

#endif /* BRAM_CTRL_H */

