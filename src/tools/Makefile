CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -std=c99 -O0 -g3 -fsanitize=undefined,address

smoke_test: smoke_test.o bram_resource.o
	$(CC) $(CFLAGS) smoke_test.o bram_resource.o -o smoke_test

bram_resource.o: bram_resource.c bram_resource.h
	$(CC) $(CFLAGS) -c bram_resource.c -o bram_resource.o

smoke_test.o: smoke_test.c
	$(CC) $(CFLAGS) -c smoke_test.c -o smoke_test.o

.PHONY: clean
clean: 
	$(RM) -f bram_resource.o
	$(RM) -f smoke_test.o
	$(RM) -f smoke_test

