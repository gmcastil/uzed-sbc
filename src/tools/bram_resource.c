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

/* 
 * TODO this should probably write to an already opened file of some sort so I
 * can dump to things like xxd instead of having to save it to a file and then
 * view the hex dump
 */
int bram_dump(struct bram_resource *bram, char *filename)
{
	int fd;
	uint32_t *map_pos = NULL;

	if (!filename) {
		fprintf(stderr, "No filename provided\n");
		return -1;
	}

	if (bram->map_size % 4) {
		fprintf(stderr, "Block RAM map sizes need to be multiples of 4 bytes\n");
		return -1;
	}
	
	fd = open(filename, O_CREAT | O_EXCL | O_RDWR, DEFAULT_BIN_CREATE_MODE);
	if (fd < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}

	map_pos = (uint32_t *) bram->map;
	for (size_t i = 0; i < bram->map_size / 4; i++) {
		printf("0x%08"PRIx32"\n", *map_pos);
		map_pos++;
	}

	close(fd);
	return 0;
	
}

/* 
 * start_addr - starting address to purge
 * stop_addr - last address to purge
 *
 * Note that since all accesses are in 32-bit chunks, that the actual address
 * that will be written to is not the same address that will be placed on the
 * hardware bus. Also, the wrong way to think about this is to specify things
 * like 0x0000 to 0x0200 to purge to 0x01ff. The former would actually purge
 * 0x0203, which causes memory corruption at best or a segmentation fault at
 * worse, if trying to purge to the upper bound.
 */
int bram_purge(struct bram_resource *bram, size_t start_addr,
		size_t stop_addr, uint32_t val)
{
	uint32_t *map_pos = NULL;
	size_t num_to_write = 0;
	int num_written = 0;
	size_t last_addr = 0;

	/* Check for NULL pointers */
	if (!bram || !bram->map) {
		fprintf(stderr, "Failed NULL pointer checks\n");
		return -1;
	}
	/* 
	 * 32-bit accesses get hard to think about - the memory is going to be
	 * treated as a large array of 32 bit values. Start and stop addresses
	 * will be masked off so that they refer to the first write and the last
	 * write in the array.  So, for example, a start address of 0x2001
	 * becomes 0x2000 and a stop address of 0x2fff becomes 0x2ffc, which
	 * would indicate a purge of the region 0x2000-0x2fff (usually the
	 * desired behavior).
	 *
	 * A possible edge case to consider - trying to use a stop address of
	 * something like 0x0100 is entirely legitimate and in that case,
	 * addresses 0x0100, 0x0101, 0x0102, and 0x0103 will all get purged. So
	 * comparisons need to be made to make sure that the relationship
	 * between the start, stop, and memory size are all correct.
	 */
	start_addr = start_addr & 0xfffffffc;
	stop_addr = stop_addr & 0xfffffffc;

	/* Guarantee we always deal with 32-bit aligned addresses */
	bad_addr_assert(start_addr % 4 == 0, "start_addr = 0x08%zu", start_addr);
	bad_addr_assert(stop_addr % 4 == 0, "stop_addr = 0x08%zu", stop_addr);
	/* 
	 * And because we do 4 byte accesses, we implicitly require block RAM
	 * to contain at least 4 bytes
	 */
	assert(bram->map_size >= 4);
	assert(bram->map_size % 4 == 0);

	/* 
	 * Since the stop address is the last address to be written to actually
	 * writes 4 bytes, we need to check that it won't go past the end of the
	 * memory. But, the map_size doesn't tell us the last address, only the
	 * size of the memory, so we explicitly calculate the largest value that
	 * the stop address can take on and actually be serviced.
	 */
	last_addr = (bram->map_size - 1) & 0xfffffffc;
	if (stop_addr > last_addr) {
		fprintf(stderr, "Stop address exceeds memory bound\n");
		return -1;
	}

	/* Do some bounds checking on start and stop addresses.*/
	if (stop_addr < start_addr) {
		fprintf(stderr, "Start address cannot be greater than stop address\n");
		return -1;
	}

	/* 
	 * Calculate the number of writes to perform - remember, these are all
	 * multiples of 4 so these kinds of comparisons are safe. The degenerate
	 * case of the start and stop addresses being the same reduces to a
	 * single 32-bit access.
	 */
	num_to_write = 1 + ((stop_addr - start_addr) >> 2);
	if ((num_to_write * 4) > bram->map_size) {
		fprintf(stderr, "Specified range is larger than resource size\n");
		return -1;
	}

	map_pos = (uint32_t *) bram->map;
	if (!map_pos) {
		fprintf(stderr, "Failed a NULL pointer check\n");
		return -1;
	}
	/* 
	 * Since we only do 32-bit accesses, we need to create a stride from the
	 * starting offset, hence the two bit shift.
	 */
	map_pos = map_pos + (start_addr >> 2);
	for (size_t i = 0; i < num_to_write; i++) {
		*map_pos = val;
		map_pos++;
		num_written++;
	}
	return num_written;
}


