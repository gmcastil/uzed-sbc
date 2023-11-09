#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

#include "bram_resource.h"
#include "bram_helper.h"

void print_usage()
{
	fprintf(stderr, "Usage: bram_load UIO MAP LOAD_ADDR FILENAME\n");
	return;
}

int load_file_to_addr(struct bram_resource *bram, FILE *file,
		uint16_t file_size, uint16_t load_addr)
{
	/*
	 * This needs to be an integer so we can properly read from fgetc() but
	 * will be cast to a uint8_t so we can write to the block RAM properly
	 */
	int rd_byte;
	uint16_t num_written;

	uint8_t *map_pos = NULL;

	if (!bram) {
		fprintf(stderr, "Error: Failed NULL pointer check\n");
		return -1;
	}

	/*
	 * First, check that the load address and the amount of data to be
	 * written are not too large
	 */
	if (file_size > (bram->map_size - load_addr)) {
		fprintf(stderr, "Error: File size too large or load address too high for "
				"block RAM\n");
		return -1;
	}

	/*View this file as a sequence of bytes since it can be any size */
	map_pos = (uint8_t *) bram->map;
	if (!map_pos) {
		fprintf(stderr, "Error: Failed NULL pointer check\n");
		return -1;
	}
	/*
	 * Addition is safe, because we've already checked the amount of data we're
	 * going to copy and cast the pointer we are going to start writing to
	 * as a pointer to bytes
	 */
	map_pos = map_pos + load_addr;
	/* Iterate over the file and treat an EOF as an aberrant condition */
	num_written = 0;
	while (num_written != file_size) {
		/*
		 * Do not use getc() here because it may be implemented as a
		 * macro that reads the stream twice and hardware may have a
		 * desire to count transactions.
		 */
		rd_byte = fgetc(file);
		if (rd_byte == EOF) {
			fprintf(stderr, "Error: Unexpected EOF\n");
			return -1;
		}
		*map_pos = (uint8_t) rd_byte;
		map_pos++;
		num_written++;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int uio_number;
	int map_number;
	uint16_t load_addr;
	char *filename;
	FILE *file;

	uint16_t file_size;
	struct bram_resource bram;

	int opt;
	int result;
	int retval;
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
	/* We error out if the file does not exist - do not create one */
	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Error: Failed NULL pointer check\n");
		return 1;
	}

	/*
	 * From here, several things can go wrong so we determine the return
	 * value for the entire program based on each successful step (getting
	 * file size, creating / destroying the block RAM resource, etc.).
	 */
	retval = 0;
	result = get_file_size(fileno(file), &file_size);
	if (result) {
		fprintf(stderr, "Error: Could not obtain file size\n");
		retval = 1;
		goto exit;	
	}

	result = bram_create(&bram, uio_number, map_number);
	if (result) {
		fprintf(stderr, "Error: Could not create block RAM resource\n");
		retval = 1;
		goto exit;
	}

	result = load_file_to_addr(&bram, file, file_size, load_addr);
	if (result) {
		fprintf(stderr, "Error: Could not load file to block RAM\n");
		retval = 1;
	}
	
	result = bram_destroy(&bram);
	if (result) {
		fprintf(stderr, "Error: Could not destroy block RAM resource\n");
		retval = 1;
		goto exit;
	}

exit:
	result = fclose(file);
	if (result) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		retval = 1;
	}

	return retval;

}
