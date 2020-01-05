#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Start fifo testing\n");

	//Create fifo
	if (!(mkfifo("fifo_file")))
		cprintf("fifo create is good\n");

	//Create fifo again
	if (mkfifo("fifo_file"))
		cprintf("fifo create again is good\n");

	//Open fifo
	int fd = open("fifo_file",O_RDONLY);
	close(fd);
	fd = open("fifo_file",O_RDONLY);

	printf("File descriptor: %d\n", fd);

}