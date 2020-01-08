#include <inc/lib.h>

void
usage(void)
{
	cprintf("usage: fifowrite [FIFO] [word]\n");
	exit();
}

void
umain(int argc, char **argv)
{

	int fd;

	if (argc != 3)
		usage();

	if ((fd = open(argv[1], O_WRONLY)) < 0)
		printf("Open error");

	printf("Write %d: \n", fprintf(fd, "%s", argv[2]));

}