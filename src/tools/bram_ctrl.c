#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "bram_ctrl.h"

#define UIO_DEV_NAME_SIZE	16	/* Max length for entries in /dev/uio */

/* Gets the path to the device node in /dev
 * Returns -1 if string is too long
 */
int bram_get_dev_path(char *dev_path, unsigned int dev_number)
{
	int result;
	result = snprintf(dev_path, UIO_DEV_NAME_SIZE, "/dev/uio%d", dev_number);
	if (result >= UIO_DEV_NAME_SIZE) {
		fprintf(stderr, "Device name too long\n");
		return -1;
	}
	return 0;
}

int bram_summary(struct bram_resource bram)
{
	printf("Device path: %s\n", bram.dev_path);
	/*
	printf("Device numbers: %d:%d\n", bram.dev_major, bram.dev_minor);
	printf("Map number: %d\n", bram.map_number);
	printf("Map addr: 0x%08"PRIx32"\n", bram.map_addr);
	printf("Map name: %s\n", bram.map_name);
	printf("Map offset: 0x%08"PRIx32"\n", bram.map_offset);
	printf("Map size: 0x%08"PRIx32"\n", bram.map_size);
	*/
	return 0;
}

struct bram_resource bram_create(unsigned int dev_number)
{
	int result;
	struct bram_resource bram;

	static char dev_path[UIO_DEV_NAME_SIZE];

	result = bram_get_dev_path(dev_path, dev_number);
	if (result) {
		fprintf(stderr, "Could not get device node name for %d\n", dev_number);
	} else {
		bram.dev_path = dev_path;
	}

	return bram;
}

int bram_destroy(struct bram_resource bram)
{
	return 0;
}

