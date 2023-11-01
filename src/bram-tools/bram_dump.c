#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#include "bram_resource.h"

void print_usage() {
	printf("Usage: bram_dump [-p] DEVICE MAP [FILENAME]\n");
	return;
}

/*
int bram_dump(struct bram_resource *bram, FILE *stream)
{
	uint32_t *map_pos = NULL;
	size_t result;

	if (!stream || !bram || !bram->map) {
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
*/
int main(int argc, char *argv[])
{
	/* bram_dump 0 1 foo.bin
	 * bram_dump -p 0 1 | xxd
	 */
	int uio_number;
	int map_number;

	struct bram_resource bram;

	bool to_stdout = false;

	char *filename;

	int opt;
	while ((opt = getopt(argc, argv, "p")) != -1) {
		switch (opt) {
			case 'p':
				to_stdout = true;
				break;
			default:
				print_usage();
				return 1;
		}
	}

	printf("%d %d\n", optind, argc);

	/* Extract the UIO number and map numbers from positional arguments */
	for (int arg_cnt = optind; arg_cnt < argc; arg_cnt++) {
		if (arg_cnt == 1) {
			uio_number = atoi(argv[arg_cnt]);
			if (uio_number < 0) {
				fprintf(stderr, "Error: Invalid UIO device number %d\n", uio_number);
				return 1;
			}
		} else if (arg_cnt == 2) {
			map_number = atoi(argv[arg_cnt]);
			if (map_number < 0) {
				fprintf(stderr, "Error: Invalid map device number %d\n", map_number);
				return 1;
			}
		} else if (arg_cnt == 3) {
			if (to_stdout) {
				fprintf(stderr, "Error: Too many positional arguments\n");
				return 1;
			} else {
				filename = argv[arg_cnt];
				printf("Output file is %s\n", filename);
			}
		} else {
			fprintf(stderr, "Too many positional arguments.\n");
			return 1;
		}
	}

	return 0;
}

