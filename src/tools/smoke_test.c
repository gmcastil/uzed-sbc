#include <stdio.h>

#include "bram_ctrl.h"

int main(int args, char *argv[])
{
	struct bram_resource bram;

	bram = bram_create(0);
	bram_summary(bram);
	bram_destroy(bram);

	return 0;
}

