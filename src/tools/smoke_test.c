#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	
	void *mem = NULL;
	/* 
	 * This will normally be either a subset of the memory in the PL or the
	 * entire range. It can and should be based upon an entry that is
	 * in /sys/devices/..../map
	 */
	size_t mem_size = 0x8000;

	fd = open("/dev/uio1", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("Could not open /dev/uio0: %s\n", strerror(errno));
		return -1;
	}
	mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, fd, 0);
	ret = munmap(mem, mem_size);

	close(fd);
	return 0;
}

