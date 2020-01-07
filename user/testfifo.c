#include <inc/lib.h>

void
umain(int argc, char **argv)
{

	//char buf[128];
	//struct Stat stat;
	//int fd1, fd2;
	int r;
	char path_fifo[] = "fifo_file";
	char string[] = "Message for fifo testing";

	cprintf("\nSART FIFO TESTING\n\n");

	//Create fifo
	if (!(mkfifo(path_fifo)))
		cprintf("1. Fifo create is good\n");

	//Create fifo again
	if (mkfifo(path_fifo) == -E_FILE_EXISTS)
		cprintf("2. Fifo create again is good\n");
/*
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

	//Wtite into closing fifo for reading
	close(fd1);
	if (write(fd2, "Hello!", 6) == -E_FIFO_OP)
		printf("11. Fifo wtite into closing fifo for reading is good\n");
	
	fd1 = open(path_fifo, O_RDONLY);

	//Read closing fifo for writing
	close(fd2);
	if (read(fd1, buf, 6) == -E_FIFO_OP)
		printf("12. Fifo read from closing fifo for writing is good\n");

	close(fd1);
*/
	//Test fifo with 2 envs
	int fifo_read_fd, fifo_write_fd;

//	mkfifo("aaa");
	
	envid_t child = fork();

  	if (child == 0) {
  		// Child

  		cprintf("Child\n");

  		char buffer[256];

  		fifo_read_fd = open(path_fifo, O_RDONLY);
	    
	    cprintf("Start of reading\n");

		while ((r = read(fifo_read_fd, buffer, 1)) > 0)
			 cprintf("Read %d: %s\n", r, buffer);

		cprintf("End of reading with %d\n", r);
	    //printf("Read %d", read(fifo_read_fd, buffer, sizeof(buffer)));
	    //while(1)
	    //	cprintf("Child\n");

/*
	    for (;;) {
	    	int num_read = read(fifo_read_fd, buffer, sizeof(buffer));
	    	if (num_read == 0) {
	        	close(fifo_read_fd);
	        	printf("End of reading\n");
	        	break;
	      	} 
	    	else {
	    		printf("Read part:  ");
	        	write(1, buffer, sizeof(buffer));
	        	printf("\n");
	     	}
	    }
*/
	} else {
		//Parent

		cprintf("Parent\n");

		fifo_write_fd = open(path_fifo, O_WRONLY);

	    cprintf("Start of writing\n");
	 	//  while(1)
	    //	cprintf("Parent\n");
	   	cprintf("Write %d", write(fifo_write_fd, string, sizeof(string)));
	   	cprintf("Write %d", write(fifo_write_fd, string, sizeof(string)));

	    cprintf("End of writing\n");

	    close(fifo_write_fd);
	    wait(child);

	}
}