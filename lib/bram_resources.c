#include <stdint.h>
#include <inttypes.h>

struct bram_resource {
	const char *name,
	uint32_t addr,
	void *mem,
	bool is_writable
};

void display_bram(struct *bram_resource)
{

}

int init_bram(struct *bram_resource)
{
	int fd = bram_resource->name

}

int destroy_bram(struct *bram_resource)
{
	int result;

	result = munmap(bram_resource->

}

int main()
{
	struct bram_resource = {
		name = "uio0",
		is_writable = false
	}
	result = init_bram(&bram_resource);
	if (result) {
		printf("Could not initialize block RAM\n");
		ret = 1;
	} else {
		display_bram(&bram_resource);
		destroy_bram(&bram_resource);
		ret =0;
	}

	return ret;
}

