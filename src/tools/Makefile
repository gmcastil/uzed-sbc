CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -ansi -O0 -g3 -fsanitize=undefined,address

smoke_test: smoke_test.o bram_ctrl.o
	$(CC) $(CFLAGS) smoke_test.o bram_ctrl.o -o smoke_test

bram_ctrl.o: bram_ctrl.c bram_ctrl.h
	$(CC) $(CFLAGS) -c bram_ctrl.c

smoke_test.o: smoke_test.c
	$(CC) $(CFLAGS) -c smoke_test.c

.PHONY: clean
clean: 
	$(RM) -f bram_ctrl.o
	$(RM) -f smoke_test.o
	$(RM) -f smoke_test

