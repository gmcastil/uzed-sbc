#include "bram_resource.h"

int bram_purge(struct bram_resource *bram, size_t start_addr,
		size_t stop_addr, uint8_t val)
{
	uint8_t *map_pos = NULL;
	size_t num_to_write = 0;
	int num_written = 0;
	size_t last_addr = 0;

	/* Check for NULL pointers */
	if (!bram || !bram->map) {
		fprintf(stderr, "Failed NULL pointer checks\n");
		return -1;
	}

	/* 
	 * Start and stop addresses should not exceed what is allowed. Note that
	 * this is what is allowed by block RAM, not the target platform
	 */
	if ((stop_addr > MAX_BRAM_ADDR) || (start_addr > MAX_BRAM_ADDR)) {
		fprintf(stderr, "Stop or start addresses are beyond the maximum allowed\n");
		return -1;
	}
	/* More address and booundary checking */
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

	/* 
	 * For single byte access, we interpret this as pointing to a bunch
	 * of 8-bit values
	 */
	map_pos = (uint8_t *) bram->map;
	if (!map_pos) {
		fprintf(stderr, "Failed a NULL pointer check\n");
		return -1;
	}

	/* Because block RAM addresses are zero indexed */
	num_to_write = 1 + (stop_addr - start_addr);
	for (size_t i = 0; i < num_to_write; i++) {
		*map_pos = val;
		map_pos++;
		num_written++;
	}
	return (int) num_written;
}

int main(int argc, char *argv[])
{

}

