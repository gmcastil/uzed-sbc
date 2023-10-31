CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -std=c99 -O0 -g3 -fsanitize=undefined,address

SRCS	:= \
		bram_dump.c \
		bram_info.c \
		bram_load.c \
		bram_peek.c \
		bram_poke.c \
		bram_purge.c \
		bram_resource.c \
		bram_search.c

# $(@D)    : $(<D)       $(^D)    directories only
# $(@F)    : $(<F)       $(^F)    files only
# $@       : $<          $^
bram_dump.o: bram_dump.c bram_resource.h
	$(CC) $(CFLAGS) 
