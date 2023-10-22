#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "bram_ctrl.h"

/* 
 * Maximum lengths for paths to /dev and /sys entries - if they are longer than
 * this something is very wrong
 */
#define UIO_DEV_PATH_SIZE	16
#define UIO_MAP_PATH_SIZE	32
#define UIO_MAX_NAME_SIZE	64

/* Gets the path to the uio device node in /dev
 * Returns -1 if string is too long
 */
int bram_get_dev_path(char *dev_path, size_t dev_path_size, unsigned int dev_number)
{
	int result;
	result = snprintf(dev_path, dev_path_size, "/dev/uio%d", dev_number);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else if (result >= dev_path_size) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	} else {
		return 0;
	}
}

/* Gets the path to the memory map location in /sys/class/uio
 * Returns -1 if the string is too long
 */
int bram_get_map_path(char *map_path, size_t map_path_size, unsigned int dev_number,
		unsigned int map_number)
{
	int result;
	result = snprintf(map_path, map_path_size,
			"/sys/class/uio/uio%d/maps/map%d", dev_number, map_number);
	if (result >= map_path_size) {
		fprintf(stderr, "Path name too logn\n");
		return -1;
	} else if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else {
		return 0;
	}
}

/* 
 * Set the physical address of the block RAM, the name from the device tree, the
 * MMIO offset value and the size of the memory available to be mapped
 */
int bram_set_mmio_addr(struct bram_resource *bram)
{
	int result;
	uint32_t map_addr;

	char filename[UIO_MAP_PATH_SIZE + 8];

	FILE *file = NULL;

	result = snprintf(filename, sizeof(filename), "%s/addr", bram->map_path);
	if (result >= sizeof(filename)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Could not open %s\n", filename);
		return -1;
	}

	result = fscanf(file, "0x%08"SCNx32"", &map_addr);
	if (result != 1) {
		fprintf(stderr, "Could not set physical address for map\n");
		return -1;
	} else {
		bram->map_addr = map_addr;
		return 0;
	}
}

int bram_set_mmio_name(struct bram_resource *bram)
{
	int result;

	char filename[UIO_MAP_PATH_SIZE + 8];

	static char map_name[UIO_MAX_NAME_SIZE];

	FILE *file = NULL;

	result = snprintf(filename, sizeof(filename), "%s/name", bram->map_path);
	if (result >= sizeof(filename)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Could not open %s\n", filename);
		return -1;
	}

	if (fgets(map_name, sizeof(map_name), file) == NULL) {
		fprintf(stderr, "Could not set name for map\n");
		return -1;
	} else {
		bram->map_name = map_name;
		return 0;
	}
}

int bram_set_mmio_attrs(struct bram_resource *bram)
{
	int result;
	result = bram_set_mmio_addr(bram);
	result = bram_set_mmio_name(bram);
	if (result) {
		fprintf(stderr, "Could not set MMIO attributes\n");
		return -1;
	}
	return 0;
}

int bram_summary(struct bram_resource bram)
{
	printf("Device path: %s\n", bram.dev_path);
	printf("Map number: %d\n", bram.map_number);
	printf("Map path: %s\n", bram.map_path);
	printf("Map addr: 0x%08"PRIx32"\n", bram.map_addr);
	printf("Map name: %s\n", bram.map_name);
	/*
	printf("Map offset: 0x%08"PRIx32"\n", bram.map_offset);
	printf("Map size: 0x%08"PRIx32"\n", bram.map_size);
	printf("Device numbers: %d:%d\n", bram.dev_major, bram.dev_minor);
	*/
	return 0;
}

struct bram_resource bram_create(unsigned int dev_number, unsigned int map_number)
{
	int result;
	struct bram_resource bram;

	static char dev_path[UIO_DEV_PATH_SIZE];
	static char map_path[UIO_MAP_PATH_SIZE];

	/* Set the location in /dev of the device node */
	result = bram_get_dev_path(dev_path, UIO_DEV_PATH_SIZE, dev_number);
	if (result) {
		fprintf(stderr, "Could not set device path for device %d\n", dev_number);
	} else {
		bram.dev_path = dev_path;
	}

	/* In the current implementation, each block RAM presents only one region to be
	 * memory mapped (i.e., map number should always be 0) */
	bram.map_number = map_number;

	/* Set the location of memory map in /sys/class/uio/uioN/maps/mapM */
	result = bram_get_map_path(map_path, UIO_MAP_PATH_SIZE, dev_number, map_number);
	if (result) {
		fprintf(stderr, "Could not set memory map path for device %d and map %d\n",
				dev_number, map_number);
	} else {
		bram.map_path = map_path;
	}

	/* Set the memory map values for this device and memory map */
	bram_set_mmio_attrs(&bram);

	return bram;
}

int bram_destroy(struct bram_resource bram)
{
	return 0;
}

