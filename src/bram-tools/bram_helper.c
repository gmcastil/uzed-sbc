#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>

#include "bram_resource.h"
#include "bram_helper.h"

/* Maximum lengths for paths to /dev and /sys entries */
#define UIO_DEV_PATH_SIZE		16
#define UIO_MAP_PATH_SIZE		32
#define UIO_MAX_MAP_NAME_SIZE		64

void print_bram_init_error(int uio_number, int map_number)
{
	fprintf(stderr, "Error: Could not create BRAM resource for UIO device %d "
			"or map number %d\n", uio_number, map_number);
	return;
}

int bram_set_dev_info(struct bram_resource *bram)
{
	int result;
	static char dev_path[UIO_DEV_PATH_SIZE];
	struct stat sb;

	result = snprintf(dev_path, sizeof(dev_path), "/dev/uio%d", bram->uio_number);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	}
	if (result >= (int) sizeof(dev_path)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}

	if (stat(dev_path, &sb) == -1) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	if (S_ISCHR(sb.st_mode)) {
		bram->dev_path = dev_path;
		bram->major = (uintmax_t) major(sb.st_rdev);
		bram->minor = (uintmax_t) minor(sb.st_rdev);
	} else {
		fprintf(stderr, "%s is not a special character device\n", dev_path);
		return -1;
	}
	return 0;
}

int bram_set_map_info(struct bram_resource *bram)
{
	int result;
	char *resultp;
	static char map_path[UIO_MAP_PATH_SIZE];

	char filepath[UIO_MAP_PATH_SIZE + 8];
	FILE *fs = NULL;

	uint32_t map_addr;
	static char map_name[UIO_MAX_MAP_NAME_SIZE];
	uint32_t map_offset;
	uint32_t map_size;

	/* Get the path to the map file in /sys which we will mmap() later */
	result = snprintf(map_path, sizeof(map_path),
			"/sys/class/uio/uio%d/maps/map%d", bram->uio_number, bram->map_number);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	}
	if (result >= (int) sizeof(filepath)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}

	/* Get the physical address of the block RAM */
	result = snprintf(filepath, sizeof(filepath), "%s/addr", map_path);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else if (result >= (int) sizeof(filepath)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}
	fs = fopen(filepath, "r");
	if (!fs) {
		fprintf(stderr, "Could not open %s\n", filepath);
		return -1;
	}
	result = fscanf(fs, "0x%08"SCNx32"", &map_addr);
	if (fclose(fs)) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	if (result != 1) {
		fprintf(stderr, "Could not get physical address for map\n");
		return -1;
	}

	/* Get the name of the map that would appear in the device tree */
	result = snprintf(filepath, sizeof(filepath), "%s/name", map_path);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else if (result >= (int) sizeof(filepath)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}
	fs = fopen(filepath, "r");
	if (!fs) {
		fprintf(stderr, "Could not open %s\n", filepath);
		return -1;
	}
	resultp = fgets(map_name, sizeof(map_name), fs);
	if (fclose(fs)) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	if (!resultp) {
		fprintf(stderr, "Could not get map name\n");
		return -1;
	}
	/*
	 * Remove the newline if it is present - note that this string is
	 * guaranteed to have a \0 at the end because that is how fgets()
	 * crafts the result
	 */
	for (int i = 0; i < (int) sizeof(map_name); i++) {
		if (map_name[i] == '\n') {
			map_name[i] = '\0';
			break;
		}
	}

	/* Get map offset */
	result = snprintf(filepath, sizeof(filepath), "%s/offset", map_path);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else if (result >= (int) sizeof(filepath)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}
	fs = fopen(filepath, "r");
	if (!fs) {
		fprintf(stderr, "Could not open %s\n", filepath);
		return -1;
	}
	result = fscanf(fs, "0x%08"SCNx32"", &map_offset);
	if (fclose(fs)) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	if (result != 1) {
		fprintf(stderr, "Could not get map offset\n");
		return -1;
	}

	/* Get map size */
	result = snprintf(filepath, sizeof(filepath), "%s/size", map_path);
	if (result < 0) {
		fprintf(stderr, "Output error\n");
		return -1;
	} else if (result >= (int) sizeof(filepath)) {
		fprintf(stderr, "Path name too long\n");
		return -1;
	}
	fs = fopen(filepath, "r");
	if (!fs) {
		fprintf(stderr, "Could not open %s\n", filepath);
		return -1;
	}
	result = fscanf(fs, "0x%08"SCNx32"", &map_size);
	if (fclose(fs)) {
		fprintf(stderr, "Error:%s\n", strerror(errno));
		return -1;
	}
	if (result != 1) {
		fprintf(stderr, "Could not get map size\n");
		return -1;
	}

	/* Now that we have all of these, we set the values */
	bram->map_path = map_path;
	bram->map_addr = map_addr;
	bram->map_name = map_name;
	bram->map_offset = map_offset;
	bram->map_size = map_size;
	return 0;
}

int bram_map_resource(struct bram_resource *bram)
{
	int fd;

	void *map;

	fd = open(bram->dev_path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		goto err_open;
	}
	/* The offset is required to be a multiple of the page size */
	if (bram->map_offset % sysconf(_SC_PAGE_SIZE)) {
		fprintf(stderr, "Map offset must be a multiple of the page size\n");
		goto err_mmap;
	}
	/*
	 * Using a temporary variable instead of assigning to bram-> allows us
	 * to not modify the struct that is passed in unless the memory map was
	 * successful
	 */
	map = mmap(NULL, bram->map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, bram->map_offset);
	if (!map) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		goto err_mmap;
	}
	close(fd);
	bram->map = map;
	return 0;

err_mmap:
	close(fd);
err_open:
	return -1;
}

int bram_unmap_resource(struct bram_resource *bram)
{
	int result;
	if (!bram->map) {
		fprintf(stderr, "No memory to unmap\n");
		return -1;
	}
	result = munmap(bram->map, bram->map_size);
	if (result) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int str_to_uint8(uint8_t *value, char *str)
{
	char *endptr = NULL;
	long result;
	int base = 16;
	int save_err;

	errno = 0;
	result = strtol(str, &endptr, base);
	save_err = errno;
	if (str == endptr) {
		fprintf(stderr, "Error: No conversion occurred\n");
		return -1;
	} else if ((save_err) == ERANGE) {
		fprintf(stderr, "Error: Resulting value out of range\n");
		return -1;
	} else if (*endptr) {
		fprintf(stderr, "Error: Invalid characters detected\n");
		return -1;
	} else if (result < 0 || result > UINT8_MAX) {
		fprintf(stderr, "Error: Negative value was received or result out of range\n");
		return -1;
	} else {
		*value = (uint8_t) result;
		return 0;
	}
}

int str_to_uint16(uint16_t *value, char *str)
{
	char *endptr = NULL;
	long result;
	int base = 16;
	int save_err;

	errno = 0;
	result = strtol(str, &endptr, base);
	save_err = errno;
	if (str == endptr) {
		fprintf(stderr, "Error: No conversion occurred\n");
		return -1;
	} else if ((save_err) == ERANGE) {
		fprintf(stderr, "Error: Resulting value out of range\n");
		return -1;
	} else if (*endptr) {
		fprintf(stderr, "Error: Invalid characters detected\n");
		return -1;
	} else if (result < 0 || result > UINT16_MAX) {
		fprintf(stderr, "Error: Negative value was received or result out of range\n");
		return -1;
	} else {
		*value = (uint16_t) result;
		return 0;
	}
}

