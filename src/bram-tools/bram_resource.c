#include <stdio.h>

#include "bram_resource.h"
#include "bram_helper.h"

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

