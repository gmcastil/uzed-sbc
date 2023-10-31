#include "bram_resource.h"

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

	/*
	 * Interpret the map as a collection of individual bytes, account for
	 * the offset, also in bytes, and then back to void since that is what
	 * fread() requires.
	 */
	map_pos = (void *) (offset + (uint8_t *) bram->map);
	if (!map_pos) {
		fprintf(stderr, "Failed NULL pointer check\n");
		return -1;
	}
	/* I do not believe I need or want to fflush() the input stream */
	result = fread(map_pos, 1, num_bytes, stream);
	if (result != num_bytes) {
		fprintf(stderr, "Stream error. Expected %zu but received %zu\n",
				num_bytes, result);
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{

}
