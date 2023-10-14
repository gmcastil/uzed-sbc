#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define SYSFS_UIO_PATH "/sys/class/uio"

struct bram_resource {
	const char *node;
	const char *node_path;
	uint32_t addr;
	uint32_t size;
	void *mem;
};

void display_bram(struct bram_resource *bram)
{
	if (bram) {
		printf("Resource name: %s\n", bram->node);
		printf("Physical address: 0x%08"PRIx32"\n", bram->addr);
	}
	return;
}

int init_bram(struct bram_resource *bram)
{
	int fd;
	int res;
	int save_errno;
	/* 
	 * We allow for paths in /sys and /dev to include uioNN - unlikely to
	 * exceed that.
	 */
	char node_path[11];
	char map_path[31];

	void *mem = NULL;

	uint32_t size;

	/* Create a path to check for the device node in /dev */
	res = snprintf(node_path, sizeof(node_path), "%s/%s", "/dev", bram->node);
	/* 
	 * If the string was truncated, it will fail the check for existence
	 * later, so we only check for output errors here.
	 */
	if (res < 0) {
		printf("Output error occurred: %s\n", strerror(errno));
		return -1;
	}
	/* 
	 * Create a path to the memory mapped region of the UIO device - note
	 * that we are assuming that there is only one map for the device. As
	 * before, if the string was truncated, it will fail the check for
	 * existence later.
	 */
	res = snprintf(map_path, sizeof(map_path), "%s/%s/maps/map0",
			SYSFS_UIO_PATH, bram->node);
	if (res < 0) {
		printf("Output error occurred: %s\n", strerror(errno));
		return -1;
	}

	/*
	 * Before we can do map the device node, we need to obtain the size of
	 * the block RAM to map. No plan to support partial mappings - we map
	 * the entire block RAM into virtual memory
	 */
	fd = open(node_path, O_RDWR | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Could not open device at %s: %s\n",
				node_path, strerror(errno));
		return -1;
	}
	size = fscanf(
	close(fd);

	/* Check if device node exists for name */
	/* Map the device node into memory */
	/* Close the file descriptor */

	/* Create a path to check for the sysfs entry */

	/* Check for sysfs entry */



	/* Add a pointer to that memory map to the bram struct */

	return 0;
}

int destroy_bram(struct bram_resource *bram)
{
	int result;

	return 0;
}

int main()
{
	int result;
	int ret;

	struct bram_resource bram = {
		.node = "uio0",
	};

	result = init_bram(&bram);
	if (result) {
		printf("Could not initialize block RAM\n");
		return 1;
	}
	display_bram(&bram);
	destroy_bram(&bram);
	return 0;
}

