#include <stdio.h>

#include "bram_resource.h"

int main(int argc, char *argv[])
{
	int result;

	struct bram_resource sbc_rom;
	struct bram_resource sbc_ram;

	result = bram_create(&sbc_rom, 0, 0);
	bram_summary(&sbc_rom);

	result = bram_create(&sbc_ram, 1, 0);
	bram_summary(&sbc_ram);

	bram_destroy(&sbc_ram);
	bram_destroy(&sbc_rom);

	return 0;
}

