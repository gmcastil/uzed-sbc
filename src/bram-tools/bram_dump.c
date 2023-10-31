#include "bram_resource.h"

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

int main(int argc, char *argv[])
{

}
