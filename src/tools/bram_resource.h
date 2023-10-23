#ifndef BRAM_CTRL_H
#define BRAM_CTRL_H

#include <stdint.h>

struct bram_resource {
	/* Path to the node created in /dev */
	char *dev_path;
	/* Path to memory map in /sys */
	char *map_path;
	/* Device major and minor numbers */
	int dev_major;
	int dev_minor;
	/* There should not be more than one map of the same resource */
	int map_number;
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

int bram_get_dev_path(char *dev_path, size_t dev_path_size,
		unsigned int dev_number);
int bram_get_map_path(char *map_path, size_t map_path_size, unsigned int dev_number,
		unsigned int map_number);
int bram_summary(struct bram_resource *bram);
struct bram_resource bram_create(unsigned int dev_number, unsigned int map_number);
int bram_destroy(struct bram_resource bram);

#endif /* BRAM_CTRL_H */

