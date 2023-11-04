#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>

#include "bram_resource.h"

void print_usage() {
	printf("Usage: bram_dump [-o outfile] DEVICE MAP\n");
	printf("\n");
	printf("Options:\n");
	printf("  %-15s%-30s\n", "-h", "display program usage");
	printf("  %-15s%-30s\n", "-o outfile", "dump to outfile instead of stdout");
	return;
}

int write_bram_data(struct bram_resource *bram, FILE *stream)
{
	uint32_t *map_pos = NULL;
	uint32_t nchunks = 0;
	size_t result = 0;

	assert(bram && stream && bram->map);

	/* Until there is a compelling reason, bulk block RAM access is 32-bit */
	if (bram->map_size % 4) {
		fprintf(stderr, "Block RAM map sizes need to be multiples of 4 bytes.\n");
		return -1;
	}

	nchunks = (bram->map_size) >> 2;
	map_pos = bram->map;
	/* Again, here we do access in 4-byte chunks */
	result = fwrite(map_pos, 4, nchunks, stream);
	if (fflush(stream)) {
		fprintf(stderr, "Failed to flush stream: %s\n", strerror(errno));
		return -1;
	}
	if (result != nchunks) {
		fprintf(stderr, "Stream error. Expected %zu transfers, but performed %zu.\n",
				nchunks, result);
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int result;
	int retval;

	char *filename = NULL;
	bool to_stdout = true;
	FILE *outfile = NULL;

	struct bram_resource bram;
	int uio_number;
	int map_number;

	int opt;
	while ((opt = getopt(argc, argv, "ho:")) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return 0;
			case 'o':
				to_stdout = false;
				/* 
				 * Save filename now, but wait to open or create it once positional
				 * arguments have been processed.
				 */
				filename = optarg;
				break;
			case '?':
				if (optopt == 'o') {
					fprintf(stderr, "No output file specified\n");
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
	/* Require two additional positional arguments */
	if ((argc - optind) != 2) {
		print_usage();
		return 1;
	} else {
		/*
		 * TODO verify that map and UIO numbers are non-negative and do
		 * not use atoi() for this
		 */
		uio_number = atoi(argv[optind]);
		map_number = atoi(argv[optind + 1]);
	}

	result = bram_create(&bram, uio_number, map_number);
	if (result) {
		fprintf(stderr, "Could not create block RAM resource for UIO device %d "
				"or map number %d\n", uio_number, map_number);
		return 1;
	}

	/* Now that we have access to block RAM resource, we can open files */
	if (to_stdout) {
		outfile = stdout;
		/* Write status to stderr so we don't pollute stdout */
		fprintf(stderr, "Dumping block RAM to stdout\n");
		/* 
		 * Many hardware error conditions manifest as all zeros on
		 * the bus as does block RAM initialization - so an indicator
		 * that some output is being sent somewhere is helpful, since
		 * dumping a block RAM post initialization to stdout, which is a very
		 * normal thing to do, would just look like a null result, which
		 * can be confusing. 
		 */
		fflush(stderr);
	} else {
		fprintf(stdout, "Dumping block RAM to %s\n", filename);
		outfile = fopen(filename, "w+");
	}
	if (!outfile) {
		fprintf(stderr, "Could not open output for writing\n");
		return 1;
	}

	/* Can have a non-zero return value for any number of reasons */
	retval = 0;
	/* Dump the entire block RAM to the output stream that was indicated */
	result = write_bram_data(&bram, outfile);
	if (result) {
		fprintf(stderr, "Could not dump block RAM resource\n");
		retval = 1;
	}
	if (bram_destroy(&bram)) {
		fprintf(stderr, "Could not destroy block RAM resource\n");
		retval = 1;
	}
	if (fclose(outfile)) {
		fprintf(stderr, "%s\n", strerror(errno));
		retval = 1;
	}
	return retval;
}

