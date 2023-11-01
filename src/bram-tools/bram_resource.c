#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>

#include "bram_resource.h"
#include "bram_helper.h"

#define bad_addr_assert(condition, ...) do { \
	if (!(condition)) { \
		fprintf(stderr, "Assertion failed: "); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, ", File: %s, Line: %d\n", __FILE__, __LINE__); \
		assert(condition); \
	} \
} while (0)

/*
 * Maximum lengths for paths to /dev and /sys entries - if they are longer than
 * this something is very wrong
 */
#define UIO_DEV_PATH_SIZE		16
#define UIO_MAP_PATH_SIZE		32
#define UIO_MAX_MAP_NAME_SIZE		64

/*
 * All binary save formats will generally need to be readable and writable by
 * user but not executable
 */
#define DEFAULT_BIN_CREATE_MODE		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
/* Block RAM addresses in this application cannot exceed this value */
#define MAX_BRAM_ADDR			0xFFFF

int bram_create(struct bram_resource *bram, int uio_number, int map_number)
{
	int result;

	if ((uio_number < 0) || (map_number < 0)) {
		fprintf(stderr, "UIO and map number must each be greater than 0\n");
		return -1;
	}
	/*
	 * These need to be set before trying to extract device ID and memory
	 * map attributes (e.g., size)
	 */
	bram->uio_number = uio_number;
	bram->map_number = map_number;
	/* Also null the memory map pointer here so it has to be set by mmap() */
	bram->map = NULL;

	/* Set path of device file to open later and device IDs */
	result = bram_set_dev_info(bram);
	if (result) {
		fprintf(stderr, "Could not set device path for device %d\n",
				bram->uio_number);
		return -1;
	}

	/* Set location and attributes for this device and memory map */
	result = bram_set_map_info(bram);
	if (result) {
		fprintf(stderr, "Could not set memory map path for device %d and map %d\n",
				bram->uio_number, bram->map_number);
		return -1;
	}

	/* Now we actually create the memory map to read and write this device */
	result = bram_map_resource(bram);
	if (result) {
		fprintf(stderr, "Could not create memory map for device %d and map %d\n",
				bram->uio_number, bram->map_number);
		return -1;
	}

	return 0;
}

int bram_destroy(struct bram_resource *bram)
{
	int result;
	if (!bram) {
		fprintf(stderr, "No block RAM resource to destroy\n");
		return -1;
	}
	result = bram_unmap_resource(bram);
	if (result) {
		fprintf(stderr, "Could not unmap resource\n");
		return -1;
	}

	return 0;
}

