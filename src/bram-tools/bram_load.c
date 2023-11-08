#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <getopt.h>
#include <string.h>

#include "bram_resource.h"
#include "bram_helper.h"

void print_usage()
{
	fprintf(stderr, "Usage: bram_load UIO MAP LOAD_ADDR FILENAME\n");
	return;
}

/*
int bram_load(struct bram_resource *bram, size_t offset,
		size_t num_bytes, FILE *stream)
{
	size_t result;
	void *map_pos = NULL;

	if (!stream || !bram || !bram->map) {
		fprintf(stderr, "Failed NULL pointer check\n");
		return -1;
	}
	if ((offset + num_bytes) > bram->map_size) {
		fprintf(stderr, "Load would exceed block RAM map size\n");
		return -1;
	}


	 * Interpret the map as a collection of individual bytes, account for
	 * the offset, also in bytes, and then back to void since that is what
	 * fread() requires.
	map_pos = (void *) (offset + (uint8_t *) bram->map);
	if (!map_pos) {
		fprintf(stderr, "Failed NULL pointer check\n");
		return -1;
	}
	* I do not believe I need or want to fflush() the input stream 
	result = fread(map_pos, 1, num_bytes, stream);
	if (result != num_bytes) {
		fprintf(stderr, "Stream error. Expected %zu but received %zu\n",
				num_bytes, result);
		return -1;
	}
	return 0;
}
*/

int main(int argc, char *argv[])
{
	int uio_number;
	int map_number;
	uint16_t load_addr;
	char *filename;

	uint16_t file_size;

	int opt;
	int result;
	int num_pos_args;
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
			default:
				print_usage();
				return 1;
		}
	}

	num_pos_args = argc - optind;
	if (num_pos_args != 4) {
		fprintf(stderr, "Error: Incorrect number of positional arguments\n");
		print_usage();
		return 1;
	}
	uio_number = atoi(argv[optind]);
	map_number = atoi(argv[optind + 1]);
	result = str_to_uint16(&load_addr, argv[optind + 2]);
	if (result) {
		fprintf(stderr, "Could not obtain load address\n");
		return 1;
	}

	filename = argv[optind + 3];

	printf("UIO = %d\n", uio_number);
	printf("map = %d\n", map_number);
	printf("load_addr = 0x%04"PRIx16"\n", load_addr);
	printf("filename = %s\n", filename);

	result = get_file_size(filename, &file_size);
	if (result) {
		fprintf(stderr, "Error: Could not obtain file size\n");
		return 1;
	}
	printf("%s size is 0x%04"PRIx16" bytes\n", filename, file_size);

	return 0;

}
