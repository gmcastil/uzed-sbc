#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fd;
	
	void *mem = NULL;

	fd = open("/dev/uio1", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("Could not open /dev/uio0: %s\n", strerror(errno));
		return -1;
	}

	close(fd);
	return 0;
}

