#include <inc/lib.h>

void
umain(int argc, char **argv)
{

	int fd;
	
	char string[] = "Hello JOS kernel!";

	if ((fd = open(argv[1], O_WRONLY)) < 0)
		printf("Open error");

	if (write(fd, string, sizeof(string)) == sizeof(string))
		printf("Write success");

}