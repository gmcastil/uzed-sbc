CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -std=c99 -O0 -g3 -fsanitize=undefined,address

bram_info: bram_info.o bram_resource.o
	$(CC) $(CFLAGS) $^ -o $@

bram_info.o: bram_info.c bram_resource.h
	$(CC) $(CFLAGS) -c $< -o $@

bram_resource.o: bram_resource.c bram_resource.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -f *.o

