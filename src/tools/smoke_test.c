#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/mman.h>

void read_bram(void *mem)
{
	uint32_t rd_data;
	rd_data = * (uint32_t *) mem;
	printf("0x%08"PRIx32"\n", rd_data);
	return;
}

int write_bram(void *mem, uint32_t wr_data)
{
	uint32_t rd_data;
	* (uint32_t *) mem = wr_data;
	rd_data = * (uint32_t *) mem;
	printf("Wrote 0x%08"PRIx32"\tRead 0x%08"PRIx32"\n", wr_data, rd_data);
	if (rd_data != wr_data) {
		return -1;
	} else {
		return 0;
	}
}

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int save_errno;

	void *mem = NULL;
	/* 
	 * This will normally be either a subset of the memory in the PL or the
	 * entire range. It can and should be based upon an entry that is
	 * in /sys/devices/..../map
	 */
	size_t mem_size = 0x8000;

	fd = open("/dev/uio0", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("Could not open /dev/uio0: %s\n", strerror(errno));
		return -1;
	}

	/* Map our UIO device into virtual memory for reading and writing */
	mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	save_errno = errno;
	close(fd);
	if (!mem) {
		printf("Could not map uio device: %s\n", strerror(save_errno));
		return -1;
	}

	read_bram(mem);
	ret = write_bram(mem, 0xdeadbeef);
	if (!ret) {
		printf("Success\n");
	}

	/* Finished with the device, so we can unmap the memory prior to exit */
	ret = munmap(mem, mem_size);
	if (ret) {
		printf("Could not unmap region: %s\n", strerror(errno));
		return -1;
	}


	close(fd);
	return 0;
}

