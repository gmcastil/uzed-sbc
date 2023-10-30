#include <stdio.h>

#include "bram_resource.h"

int main(int argc, char *argv[])
{
	int result;

	size_t start_addr = 0x0000;
	size_t stop_addr = 0x01ff;

	/* 
	 * In hardware, uio0 is instrumented on both sides of the block RAM 
	 * which corresponds to address 0x40000000
	 */
	struct bram_resource sbc_ram;

	if (bram_create(&sbc_ram, 0, 0)) {
		return 1;
	}

	bram_summary(&sbc_ram);

	result = bram_purge(&sbc_ram, start_addr, stop_addr, 0xff);
	bram_dump(&sbc_ram, "test.bin");

	bram_destroy(&sbc_ram);

	return 0;
}

