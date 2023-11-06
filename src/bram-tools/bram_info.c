#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include "bram_helper.h"
#include "bram_resource.h"

void print_usage() {
	printf("Usage: bram_info DEVICE MAP\n");
	return;
}

int print_bram_summary(struct bram_resource *bram)
{
	if (!bram) {
		fprintf(stderr, "Error: Failed a NULL pointer check\n");
		return -1;
	}
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

int main(int argc, char *argv[])
{
	int uio_number;
	int map_number;

	struct bram_resource bram;

	/* Do not accept any options */
	int opt;
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
			default:
				print_usage();
				return 1;
		}
	}
	/* Require both the UIO device and map numbers to be provided */
	if ((optind + 2) != argc) {
		print_usage();
		return 1;
	}

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
		} else {
			fprintf(stderr, "Too many positional arguments.\n");
			return 1;
		}
	}

	if (bram_create(&bram, uio_number, map_number)) {
		print_bram_init_error(uio_number, map_number);
		return 1;
	}
	print_bram_summary(&bram);
	bram_destroy(&bram);

	return 0;
}

