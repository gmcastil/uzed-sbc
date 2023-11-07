#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

#include "bram_resource.h"
#include "bram_helper.h"

/* Checkerboard patterns begin with this and complement it each write */
#define XBOARD_START		0x55

int purge_bram_by_value(struct bram_resource *bram, uint16_t start_addr,
		uint16_t stop_addr, uint8_t purge_val)
{
	uint8_t *map_pos = NULL;
	uint16_t num_to_write = 0;
	uint16_t num_written = 0;

	map_pos = bram->map;
	if (!map_pos) {
		fprintf(stderr, "Error: NULL memory map\n");
		return -1;
	}
	printf("Purging 0x%04"PRIx16" to 0x%04"PRIx16" with value 0x%02"PRIx8"\n",
			start_addr, stop_addr, purge_val);
	num_to_write = 1 + (stop_addr - start_addr);
	for (int i = 0; i < num_to_write; i++) {
		*map_pos = purge_val;
		map_pos++;
		num_written++;
	}
	return (num_to_write - num_written);
}

int purge_bram_by_xboard(struct bram_resource *bram, uint16_t start_addr,
		uint16_t stop_addr)
{
	uint8_t purge_val = XBOARD_START;
	uint8_t *map_pos = NULL;
	uint16_t num_to_write = 0;
	uint16_t num_written = 0;

	map_pos = bram->map;
	if (!map_pos) {
		fprintf(stderr, "Error: NULL memory map\n");
		return -1;
	}
	printf("Purging 0x%04"PRIx16" to 0x%04"PRIx16" with checkerboard pattern\n",
			start_addr, stop_addr);
	num_to_write = 1 + (stop_addr - start_addr);
	for (int i = 0; i < num_to_write; i++) {
		*map_pos = purge_val;
		map_pos++;
		purge_val = ~purge_val;
		num_written++;
	}
	return (num_to_write - num_written);
}

int purge_bram_by_incr(struct bram_resource *bram, uint16_t start_addr,
		uint16_t stop_addr)
{
	/*
	 * We are going to pull the 8-bits from the address we start with and
	 * then increment from there, relying on the fact that unsigned overflow
	 * is defined so that the pattern we see in hardware wraps around as
	 * expected.
	 */
	uint8_t purge_val;
	uint8_t *map_pos = NULL;
	uint16_t num_to_write = 0;
	uint16_t num_written = 0;

	map_pos = bram->map;
	if (!map_pos) {
		fprintf(stderr, "Error: NULL memory map\n");
		return -1;
	}
	printf("Purging 0x%04"PRIx16" to 0x%04"PRIx16" with incrementing pattern\n",
			start_addr, stop_addr);

	num_to_write = 1 + (stop_addr - start_addr);
	purge_val = (start_addr & 0x00ffU) >> 8;
	for (int i = 0; i < num_to_write; i++) {
		*map_pos = purge_val;
		map_pos++;
		purge_val++;
		num_written++;
	}
	return (num_to_write - num_written);
}

/*
int purge_by_val(struct bram_resource *bram, size_t start_addr,
		size_t stop_addr, uint8_t val)
{
	uint8_t *map_pos = NULL;
	size_t num_to_write = 0;
	int num_written = 0;
	size_t last_addr = 0;

	if (!bram || !bram->map) {
		fprintf(stderr, "Failed NULL pointer checks\n");
		return -1;
	}

	if ((stop_addr > MAX_BRAM_ADDR) || (start_addr > MAX_BRAM_ADDR)) {
		fprintf(stderr, "Stop or start addresses are beyond the maximum allowed\n");
		return -1;
	}
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

	map_pos = (uint8_t *) bram->map;
	if (!map_pos) {
		fprintf(stderr, "Failed a NULL pointer check\n");
		return -1;
	}

	num_to_write = 1 + (stop_addr - start_addr);
	for (size_t i = 0; i < num_to_write; i++) {
		*map_pos = val;
		map_pos++;
		num_written++;
	}
	return (int) num_written;
}
*/

void print_usage()
{
	printf("Usage: bram_purge [-x] [-i] [-v VALUE] DEVICE MAP [START [END]]\n");
	printf("\n");
	printf("Options:\n");
	printf("  %-15s%-30s\n", "-h", "display program usage");
	printf("  %-15s%-30s\n", "-x", "purge with checkerboard pattern");
	printf("  %-15s%-30s\n", "-i", "purge with incrementing pattern");
	printf("  %-15s%-30s\n", "-v VALUE", "purge with value");
	printf("\n");

	return;
}

int main(int argc, char *argv[])
{
	int uio_number;
	int map_number;
	uint8_t purge_val;
	uint16_t start_addr;
	uint16_t stop_addr;

	bool start_given;
	bool stop_given;

	bool by_incr = false;
	bool by_xboard = false;
	bool by_value = false;

	int result;
	int retval;
	struct bram_resource bram;
	int num_pos_args;

	int opt;
	while ((opt = getopt(argc, argv, "hixv:")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return 0;
			case 'i':
				by_incr = true;
				break;
			case 'x':
				by_xboard = true;
				break;
			case 'v':
				by_value = true;
				if (str_to_uint8(&purge_val, optarg)) {
					fprintf(stderr, "Error: Bad purge value\n");
					return 1;
				}
				break;
			case '?':
				if (optopt == 'v') {
					fprintf(stderr, "Error: No purge value specified\n");
				} else if (isprint(optopt)) {
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				} else {
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				}
				return 1;
			default:
				print_usage();
				return 1;
		}
	}

	/* Require at least 2 and at most 4 positional arguments */
	num_pos_args = argc - optind;
	if ((num_pos_args < 2) || (num_pos_args > 4)) {
		fprintf(stderr, "Error: Incorrect number of positional arguments\n");
		print_usage();
		return 1;
	}
	/* TODO Sanitize these two values */
	uio_number = atoi(argv[optind]);
	map_number = atoi(argv[optind + 1]);

	/* Process the remaining 0 to 2 positional arguments */
	switch (num_pos_args) {
		/* No start or end address given */
		case 2:
			start_given = false;
			stop_given = false;
			break;
		/* Only given starting address */
		case 3:
			if (str_to_uint16(&start_addr, argv[optind + 2])) {
				fprintf(stderr, "Error: Bad starting address\n");
				return -1;
			}
			start_given = true;
			stop_given = false;
			break;
		/* Start and end addresses given */
		case 4:
			if (str_to_uint16(&start_addr, argv[optind + 2])) {
				fprintf(stderr, "Error: Bad starting address\n");
				return -1;
			}
			if (str_to_uint16(&stop_addr, argv[optind + 3])) {
				fprintf(stderr, "Error: Bad ending address\n");
				return -1;
			}
			start_given = true;
			stop_given = true;
			break;

		/* Too many positional arguments given */
		default:
			fprintf(stderr, "Error: Too many positional arguments\n");
			print_usage();
			return 1;
	}
	result = bram_create(&bram, uio_number, map_number);
	if (result) {
		fprintf(stderr, "Could not create block RAM resource for UIO device %d "
				"or map number %d\n", uio_number, map_number);
		return 1;
	}

	/* Need start and stop addresses before purge can begin */
	if (!stop_given) {
		stop_addr = bram.map_size - 1;
	}
	if (!start_given) {
		start_addr = 0;
	}

	/* If bounds check before purging fails, we still have to exit cleanly */
	if (start_addr > stop_addr) {
		fprintf(stderr, "Error: Start address is greater than stop address\n");
		retval = 1;
		goto err_exit;
	}
	if (stop_addr > (bram.map_size - 1)) {
		fprintf(stderr, "Error: Stop address cannot exceed map size\n");
		retval = 1;
		goto err_exit;
	}
	/*
	 * It's too annoying to try to manage the case where multiple purge
	 * patterns are selected, so just select them in the order they're
	 * checked for here. If the user hasn't selected one, then we just fall
	 * out the bottom with nothing to do.
	 */
	if (by_value) {
		retval = purge_bram_by_value(&bram, start_addr, stop_addr, purge_val);
	} else if (by_xboard) {
		retval = purge_bram_by_xboard(&bram, start_addr, stop_addr);
	} else if (by_incr) {
		retval = purge_bram_by_incr(&bram, start_addr, stop_addr);
	} else {
		printf("No purge pattern selected. Exiting\n");
		retval = 1;
	}

err_exit:
	if (bram_destroy(&bram)) {
		fprintf(stderr, "Could not destroy block RAM resource\n");
		retval = 1;
	}
	/* Either we failed a bounds check or purging was unsuccessful */
	return retval;

}

