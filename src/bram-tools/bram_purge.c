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

/*
int bram_purge(struct bram_resource *bram, size_t start_addr,
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
	printf("Usage: bram_purge DEVICE MAP VALUE [START [END]]\n");
	return;
}

int main(int argc, char *argv[])
{
	int uio_number;
	int map_number;
	uint8_t purge_val;
	uint16_t start_addr;
	uint16_t end_addr;

	bool start_given;
	bool end_given;

	int opt;
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
			default:
				print_usage();
				return 1;
			}
	}

	if ((argc - optind) < 3) {
		print_usage();
		return 1;
	} else {
		uio_number = atoi(argv[optind]);
		map_number = atoi(argv[optind + 1]);
		if (str_to_uint8_t(&purge_val, argv[optind + 2])) {
			fprintf(stderr, "Error Bad purge value\n");
			return -1;
		}
		/* This should never be able to occur */
		assert(argc >= (optind - 3));
		printf("argc = %d\n", argc);
		printf("optind = %d\n", optind);
		/* Still need to validate addresses against the actual block RAM */
		switch (argc - optind - 3) {
			/* No start or end address given - extract later */
			case 0:
				start_given = false;
				end_given = false;
				break;
			/* Only given starting address - extract end later */
			case 1:
				if (str_to_uint16_t(&start_addr, argv[optind + 3])) {
					fprintf(stderr, "Error: Bad starting address\n");
					return -1;
				}
				start_given = true;
				end_given = false;
				break;
			/* Start and end addresses given */
			case 2:
				if (str_to_uint16_t(&start_addr, argv[optind + 3])) {
					fprintf(stderr, "Error: Bad starting address\n");
					return -1;
				}
				if (str_to_uint16_t(&end_addr, argv[optind + 4])) {
					fprintf(stderr, "Error: Bad ending address\n");
					return -1;
				}
				start_given = true;
				end_given = true;
				break;

			/* Too many positional arguments given */
			default:
				fprintf(stderr, "Error: Too many positional arguments\n");
				print_usage();
				return 1;
		}
	}

	printf("uio = %d\n", uio_number);
	printf("map = %d\n", map_number);
	printf("purge = 0x%02"PRIx8"\n", purge_val);
	if (start_given) {
		printf("start = 0x%04"PRIx16"\n", start_addr);
	}
	if (end_given) {
		printf("end = 0x%04"PRIx16"\n", end_addr);
	}

}

