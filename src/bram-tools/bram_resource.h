#ifndef BRAM_CTRL_H
#define BRAM_CTRL_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

/* This will typically be determined by the PS configuration within Vivado */
#define BRAM_AXI_CTRL_WIDTH			32

struct bram_resource {
	/* User provides the UIO device and map numbers at creation */
	int uio_number;
	int map_number;
	/* Path to the node created in /dev */
	char *dev_path;
	/* Device major and minor numbers */
	unsigned int major;
	unsigned int minor;
	/* Path to memory map in /sys */
	char *map_path;
	/* Physical address of the block RAM */
	uint32_t map_addr;
	/* String identifier for the mapping */
	char *map_name;
	/* 
	 * Location where UIO device has been mapped in memory - this is the
	 * location returned by call to mmap() that has to be unmapped when the
	 * BRAM resource is destroyed
	 */
	void *map;
	/*
	 * Offset in bytes that has to be added to mmap() return to get to the
	 * actual device memory. This is usually zero and will be checked to
	 * make sure that it is a multiple of the page size as returned by
	 * sysconf(_SC_PAGE_SIZE).
	 */
	off_t map_offset;
	/* 
	 * Memory depth - NOTE this is the range as determined by the
	 * address editor in Vivado, not the total size of the block RAM.
	 */
	size_t map_size;
	/*
	 * The device node for the block RAM controller has the property
	 * `xlnx,s-axi-ctrl-data-width` which specifies the AXI bus width, but
	 * this may not presently be exported by the UIO driver. For now, we
	 * will set it at compile time and then carry it around with the BRAM
	 * resource structure.
	 */
	size_t map_width;
};

int bram_create(struct bram_resource *bram, int uio_number, int map_number);
int bram_destroy(struct bram_resource *bram);
#endif /* BRAM_CTRL_H */

