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

/* Function prototypes */
int bram_set_dev_info(struct bram_resource *bram);
int bram_set_map_info(struct bram_resource *bram);

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

int bram_summary(struct bram_resource *bram)
{
	printf("%-16s%s\n", "Device path:", bram->dev_path);
	printf("%-16s%d:%d\n", "Device numbers:", bram->major, bram->minor);
	printf("%-16s%d\n", "Map number:", bram->map_number);
	printf("%-16s0x%"PRIxPTR"\n", "Map pointer:", (uintptr_t) bram->map);
	printf("%-16s%s\n", "Map path:", bram->map_path);
	printf("%-16s0x%08"PRIx32"\n", "Map addr:", bram->map_addr);
	printf("%-16s%s\n", "Map name:", bram->map_name);
	printf("%-16s0x%08"PRIx32"\n", "Map offset:", (uint32_t) bram->map_offset);
	printf("%-16s0x%08"PRIx32"\n", "Map size:", (uint32_t) bram->map_size);
	return 0;
}

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

int bram_dump(struct bram_resource *bram, FILE *stream)
{
	uint32_t *map_pos = NULL;
	size_t result;

	if (!stream) {
		fprintf(stderr, "Failed NULL pointer check\n");
		return -1;
	}

	if (bram->map_size % 4) {
		fprintf(stderr, "Block RAM map sizes need to be multiples of 4 bytes\n");
		return -1;
	}
	
	map_pos = bram->map;
	result = fwrite(map_pos, 1, bram->map_size, stream);
	if (fflush(stream)) {
		fprintf(stderr, "fflush() failed: %s\n", strerror(errno));
	}
	if (result != bram->map_size) {
		fprintf(stderr, "Stream error. Expected %zu but received %zu\n",
				bram->map_size, result);
		return -1;
	}

	return 0;
}

int bram_purge(struct bram_resource *bram, size_t start_addr,
		size_t stop_addr, uint8_t val)
{
	uint8_t *map_pos = NULL;
	size_t num_to_write = 0;
	int num_written = 0;
	size_t last_addr = 0;

	/* Check for NULL pointers */
	if (!bram || !bram->map) {
		fprintf(stderr, "Failed NULL pointer checks\n");
		return -1;
	}

	/* 
	 * Start and stop addresses should not exceed what is allowed. Note that
	 * this is what is allowed by block RAM, not the target platform
	 */
	if ((stop_addr > MAX_BRAM_ADDR) || (start_addr > MAX_BRAM_ADDR)) {
		fprintf(stderr, "Stop or start addresses are beyond the maximum allowed\n");
		return -1;
	}
	/* More address and booundary checking */
	if (stop_addr < start_addr) {
		fprintf(stderr, "Ending address must be greater than or equal to the "
				"starting address\n");
		return -1;
	}
	last_addr = bram->map_size - 1;
	if (stop_addr > last_addr) {
		fprintf(stderr, "Ending address must not exceed memory upper bound\n");
		return -1;
	}

	/* 
	 * For single byte access, we interpret this as pointing to a bunch
	 * of 8-bit values
	 */
	map_pos = (uint8_t *) bram->map;
	if (!map_pos) {
		fprintf(stderr, "Failed a NULL pointer check\n");
		return -1;
	}

	/* Because block RAM addresses are zero indexed */
	num_to_write = 1 + (stop_addr - start_addr);
	for (size_t i = 0; i < num_to_write; i++) {
		*map_pos = val;
		map_pos++;
		num_written++;
	}
	return (int) num_written;
}

