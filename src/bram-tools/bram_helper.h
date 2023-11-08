#ifndef BRAM_HELPER_H
#define BRAM_HELPER_H

#include "bram_resource.h"

void print_bram_init_error(int uio_number, int map_number);
int bram_set_dev_info(struct bram_resource *bram);
int bram_set_map_info(struct bram_resource *bram);
int bram_map_resource(struct bram_resource *bram);
int bram_unmap_resource(struct bram_resource *bram);

/* Useful functions for validating input */
int str_to_uint8(uint8_t *value, char *str);
int str_to_uint16(uint16_t *value, char *str);

/* Other common operations */
int get_file_size(char *filename, uint16_t *size);

#endif /* BRAM_HELPER_H */
