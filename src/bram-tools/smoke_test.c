#include <stdio.h>

#include "bram_resource.h"

int main(int argc, char *argv[])
{
	int result;

	size_t start_addr = 0x0000;
	size_t stop_addr = 0x3fff;

	/* 
	 * In hardware, uio0 is instrumented on both sides of the block RAM 
	 * which corresponds to address 0x40000000
	 */
	struct bram_resource sbc_ram;

	if (bram_create(&sbc_ram, 1, 0)) {
		return 1;
	}

	bram_dump(&sbc_ram, stdout);
	result = bram_purge(&sbc_ram, start_addr, stop_addr, 0xff);
	if (result != (int) (stop_addr - start_addr)) {
		fprintf(stderr, "Purge error\n");
		return -1;
	}
	bram_dump(&sbc_ram, stdout);

	start_addr = 0x0100;
	stop_addr = 0x01ff;
	result = bram_purge(&sbc_ram, start_addr, stop_addr, 0x00);
	if (result != (int) (stop_addr - start_addr)) {
		fprintf(stderr, "Purge error\n");
		return -1;
	}
	bram_dump(&sbc_ram, stdout);

	bram_destroy(&sbc_ram);

	return 0;
}

