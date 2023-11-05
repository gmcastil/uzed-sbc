#ifndef BRAM_HELPER_H
#define BRAM_HELPER_H

#include "bram_resource.h"

void print_bram_init_error(int uio_number, int map_number);
int bram_set_dev_info(struct bram_resource *bram);
int bram_set_map_info(struct bram_resource *bram);
int bram_map_resource(struct bram_resource *bram);
int bram_unmap_resource(struct bram_resource *bram);

#endif /* BRAM_HELPER_H */