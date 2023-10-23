#include <stdio.h>

#include "bram_resource.h"

int main(int argc, char *argv[])
{
	struct bram_resource sbc_rom;
	struct bram_resource sbc_ram;

	sbc_rom = bram_create(0, 0);
	bram_summary(&sbc_rom);

	sbc_ram = bram_create(1, 0);
	bram_summary(&sbc_ram);

	bram_destroy(sbc_ram);
	bram_destroy(sbc_rom);

	return 0;
}

