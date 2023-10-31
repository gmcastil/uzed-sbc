#include "bram_resource.h"

int bram_summary(struct bram_resource *bram)
{
	printf("%-16s%s\n", "Device path:", bram->dev_path);
	printf("%-16s%d:%d\n", "Device numbers:", bram->major, bram->minor);
	printf("%-16s%d\n", "Map number:", bram->map_number);
	printf("%-16s0x%"PRIxPTR"\n", "Map pointer:", (uintptr_t) bram->map);
	printf("%-16s%s\n", "Map path:", bram->map_path);
	printf("%-16s0x%08"PRIx32"\n", "Map addr:", bram->map_addr);
	printf("%-16s%s\n", "Map name:", bram->map_name);
	printf("%-16s0x%08"PRIx32"\n", "Map offset:", (uint32_t) bram->map_offset);
	printf("%-16s0x%08"PRIx32"\n", "Map size:", (uint32_t) bram->map_size);
	return 0;
}

int main(int argc, char *argv[])
{

}
