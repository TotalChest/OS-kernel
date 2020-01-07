#include <inc/lib.h>

void
umain(int argc, char **argv)
{

	char buf[64];
	struct Stat stat;
	int fd1, fd2;
	int r;
	char path_fifo[] = "fifo_file";
	char string[] = "Message for testing FIFO files";

	cprintf("\nSART FIFO TESTING\n\n");

	//Create fifo
	if (!(mkfifo(path_fifo)))
		cprintf("1. Fifo create is good\n");

	//Create fifo again
	if (mkfifo(path_fifo) == -E_FILE_EXISTS)
		cprintf("2. Fifo create again is good\n");

	//Open fifo
	if ((fd1 = open(path_fifo, O_RDONLY)) > 0)
		printf("3. Fifo open with descriptor fd1: %d\n", fd1);
	if ((fd2 = open(path_fifo, O_WRONLY)) > 0)
		printf("4. Fifo open with descriptor fd2: %d\n", fd2);

	//Get stat
	if (!(fstat(fd1, &stat)))
		printf("5. Stat fd1: %s %d\n", stat.st_name, stat.st_isfifo);
	if (!(fstat(fd2, &stat)))
		printf("6. Stat fd2: %s %d\n", stat.st_name, stat.st_isfifo);

	//Write into fifo
	if (write(fd2, "Hello!", 6) == 6)
		printf("7. Fifo write is good\n");

	//Read from fifo
	if (read(fd1, buf, 6) == 6)
		printf("8. Fifo read: %s\n", buf);

	//Write in RDONLY discriptor
	if (write(fd1, "Hello!", 6) == -E_INVAL)
		printf("9. Fifo mode RDONLY is good\n");

	//Read from WRONLY discriptor
	if (read(fd2, buf, 6) == -E_INVAL)
		printf("10. Fifo mode WRONLY is good\n");

	if (!close(fd1))
		printf("11. Fifo fd1 close is good\n");

	if (!close(fd2))
		printf("12. Fifo fd2 close is good\n");



	//Test fifo with 2 envs

	int fifo_read_fd, fifo_write_fd;

	fifo_read_fd = open(path_fifo, O_RDONLY);
	
	envid_t child = fork();

  	if (child == 0) {

  		// Child

  		cprintf("Child\n");

		while ((r = read(fifo_read_fd, buf, 10)) > 0)
			cprintf("Read from fifo %d bytes: %s\n", r, buf);

		cprintf("End of reading with code %d\n", r);

		close(fifo_read_fd);

	} 
	else {

		//Parent

		cprintf("Parent\n");

		close(fifo_read_fd);

		fifo_write_fd = open(path_fifo, O_WRONLY);

	   	r = write(fifo_write_fd, string, sizeof(string));

	    cprintf("End of writing with code %d\n", r);

	    close(fifo_write_fd);

	    wait(child);

	}
}