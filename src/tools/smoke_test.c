#include <stdio.h>

#include "bram_ctrl.h"

int main(int argc, char *argv[])
{
	struct bram_resource bram;

	bram = bram_create(0);
	bram_summary(bram);
	bram_destroy(bram);

	return 0;
}

