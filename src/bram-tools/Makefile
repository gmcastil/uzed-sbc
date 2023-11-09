CC	:= /usr/bin/gcc
CFLAGS	:= -Wall -pedantic -Wextra -O0 -g3 -fsanitize=undefined,address
LDFLAGS := -fsanitize=undefined,address

.PHONY: all
all: bram_info bram_dump bram_purge bram_load

bram_info: bram_info.o bram_resource.o bram_helper.o
	$(CC) $(LDFLAGS) $^ -o $@

bram_dump: bram_dump.o bram_resource.o bram_helper.o
	$(CC) $(LDFLAGS) $^ -o $@

bram_purge: bram_purge.o bram_resource.o bram_helper.o
	$(CC) $(LDFLAGS) $^ -o $@

bram_load: bram_load.o bram_resource.o bram_helper.o
	$(CC) $(LDFLAGS) $^ -o $@

bram_info.o: bram_info.c bram_resource.h
	$(CC) $(CFLAGS) -std=c99 -c $< -o $@

bram_dump.o: bram_dump.c bram_resource.h
	$(CC) $(CFLAGS) -std=c99 -c $< -o $@

bram_purge.o: bram_purge.c bram_resource.h
	$(CC) $(CFLAGS) -std=c99 -c $< -o $@

bram_load.o: bram_load.c bram_resource.h
	$(CC) $(CFLAGS) -D__USE_POSIX -c $< -o $@

bram_resource.o: bram_resource.c bram_resource.h bram_helper.h
	$(CC) $(CFLAGS) -std=c99 -c $< -o $@

bram_helper.o: bram_helper.c bram_resource.h bram_helper.h
	$(CC) $(CFLAGS) -std=c99 -c $< -o $@

.PHONY: clean
clean:
	$(RM) -f *.o
	$(RM) bram_info bram_dump bram_purge bram_load

