CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -ansi -O0 -g

bram_resources: bram_resources.c
	$(CC) $(CFLAGS) -std=c99 $< -o $@

.PHONY: clean
clean: 
	$(RM) -f bram_resource

