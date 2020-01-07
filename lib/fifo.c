#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>

#ifdef debug
#undef debug
#endif

#define debug 0

union Fsipc fsipcbuf __attribute__((aligned(PGSIZE)));

static int
fsipc_fifo(unsigned type, void *dstva)
{
	static envid_t fsenv;
	if (fsenv == 0)
		fsenv = ipc_find_env(ENV_TYPE_FS);

	static_assert(sizeof(fsipcbuf) == PGSIZE, "Invalid fsipcbuf size");

	if (debug)
		cprintf("[%08x] fsipc %d %08x\n", thisenv->env_id, type, *(uint32_t *)&fsipcbuf);

	ipc_send(fsenv, type, &fsipcbuf, PTE_P | PTE_W | PTE_U);
	return ipc_recv(NULL, dstva, NULL);
}

static int devfifo_close(struct Fd *fd);
static ssize_t devfifo_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfifo_write(struct Fd *fd, const void *buf, size_t n);
static int devfifo_stat(struct Fd *fd, struct Stat *stat);

struct Dev devfifo =
{
	.dev_id =	'o',
	.dev_name =	"fifo",
	.dev_read =	devfifo_read,
	.dev_stat =	devfifo_stat,
	.dev_close =	devfifo_close,
	.dev_write =	devfifo_write,
};

int
mkfifo(const char *path)
{
	int r;

	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;

	strcpy(fsipcbuf.create_fifo.req_path, path);

	if ((r = fsipc_fifo(FSREQ_CREATE_FIFO, NULL)) < 0) {
		return r;
	}

	return 0;
}

int
get_fifo(struct Fifo *fifo, int id){

	int r;

	fsipcbuf.get_set_fifo.req_fileid = id;
	if ((r = fsipc_fifo(FSREQ_GET_FIFO, NULL)) < 0)
		cprintf("get_fifo: %i", r);
	fifo->n_readers = fsipcbuf.get_set_fifo.n_readers;
	fifo->n_writers = fsipcbuf.get_set_fifo.n_writers;
	fifo->fifo_rpos = fsipcbuf.get_set_fifo.fifo_rpos;
	fifo->fifo_wpos = fsipcbuf.get_set_fifo.fifo_wpos;
	memmove(fifo->fifo_buf, fsipcbuf.get_set_fifo.fifo_buf, FIFOBUFSIZ);
	return r;
}

int
set_fifo(struct Fifo fifo, int id){

	int r;

	fsipcbuf.get_set_fifo.req_fileid = id;
	fsipcbuf.get_set_fifo.n_readers = fifo.n_readers;
	fsipcbuf.get_set_fifo.n_writers = fifo.n_writers;
	fsipcbuf.get_set_fifo.fifo_rpos = fifo.fifo_rpos;
	fsipcbuf.get_set_fifo.fifo_wpos = fifo.fifo_wpos;
	memmove(fsipcbuf.get_set_fifo.fifo_buf, fifo.fifo_buf, FIFOBUFSIZ);
	if ((r = fsipc_fifo(FSREQ_SET_FIFO, NULL)) < 0)
		cprintf("set_fifo: %i", r);
	return r;
}


static ssize_t
devfifo_read(struct Fd *fd, void *buf, size_t n)
{
	int r;
	//int yield;
	//uint8_t *buf;
	int c = 0;
	
	
	while(1){
		//sys_yield();
		//cprintf("read plan\n");
		fsipcbuf.read_fifo.req_fileid = fd->fd_file.id;
		fsipcbuf.read_fifo.req_n = n - c;

		//fsipcbuf.read_fifo.req_fileid = fd->fd_file.id;
		//fsipcbuf.read_fifo.req_n = n - c;

		if ((r = fsipc_fifo(FSREQ_READ_FIFO, NULL)) < 0){
			if (r == -E_FIFO){
				memmove(buf + c, fsipcbuf.readRet.ret_buf, fsipcbuf.readRet.ret_n);
				c += fsipcbuf.readRet.ret_n;
				cprintf("read plan\n");
				sys_yield();
				continue;
			}

			return r;
		}
		//assert(r <= n);
		//assert(r <= PGSIZE);
		
		memmove(buf + c, fsipcbuf.readRet.ret_buf, r);

		c += r;
		//yield = fsipcbuf.readRet.ret_yield;
		//c += r;
		break;
	}

	return c;

/*
	buf = vbuf;
	struct Fifo fifo;
	get_fifo(&fifo, fd->fd_file.id);

	for (i = 0; i < n; i++) {
		cprintf("read %d\n %d\n", i,fifo.fifo_rpos);
		get_fifo(&fifo, fd->fd_file.id);
		while (fifo.fifo_rpos == fifo.fifo_wpos) {
			//empty
			cprintf("read cicle\n");
			if (i > 0)
				return i;

			get_fifo(&fifo, fd->fd_file.id);
			if (fifo.n_writers == 0)//помогло убрать зацикливания
				return 0;

			//set_fifo(fifo, fd->fd_file.id);
			sys_yield();
			get_fifo(&fifo, fd->fd_file.id);
		}
		get_fifo(&fifo, fd->fd_file.id);
		buf[i] = fifo.fifo_buf[fifo.fifo_rpos % FIFOBUFSIZ];
		fifo.fifo_rpos++;
		set_fifo(fifo, fd->fd_file.id);
	}

	return i;
	*/
}

static ssize_t
devfifo_write(struct Fd *fd, const void *buf, size_t n)
{
	uint32_t max;
	int r = 0;
	//const uint8_t *buf;

	if (n > (max = PGSIZE - (sizeof(int) + sizeof(size_t))))
		n = max;

	fsipcbuf.write_fifo.req_fileid = fd->fd_file.id;
	
	while(1){
	//do{	
		//sys_yield();
		//cprintf("write plan\n");

		fsipcbuf.write_fifo.req_n = n;
		memmove(fsipcbuf.write_fifo.req_buf, buf, n);

		if ((r = fsipc_fifo(FSREQ_WRITE_FIFO, NULL)) < 0){
			//if (r > 0){
			//	cprintf("write plan\n");
			//	sys_yield();
			//	continue;
			//}
			//if (r == -E_FIFO){
			//	cprintf("write plan\n");
			//	sys_yield();
			//	continue;
			//}
			return r;
			}

		break;
	}
		//c += r;
//	} while(c != n);

	return r;

/*
	//fifo_state.write++;

	buf = vbuf;
	struct Fifo fifo;
	get_fifo(&fifo, fd->fd_file.id);

	for (i = 0; i < n; i++) {
		cprintf("write %d\n %d\n", i,fifo.fifo_wpos);
		get_fifo(&fifo, fd->fd_file.id);
		while (fifo.fifo_wpos >= fifo.fifo_rpos + sizeof(fifo.fifo_buf)) {
			//full
			cprintf("write cicle\n");

			get_fifo(&fifo, fd->fd_file.id);
			if (fifo.n_readers == 0){
				//fifo_state.write--;
				return 0;
			}

			sys_yield();
			get_fifo(&fifo, fd->fd_file.id);
		}
		fifo.fifo_buf[fifo.fifo_wpos % FIFOBUFSIZ] = buf[i];
		fifo.fifo_wpos++;
		set_fifo(fifo, fd->fd_file.id);
	}

	//fifo_state.write--;
	return i;
	*/
}

static int
devfifo_stat(struct Fd *fd, struct Stat *st)
{
	int r;

	fsipcbuf.stat_fifo.req_fileid = fd->fd_file.id;
	if ((r = fsipc_fifo(FSREQ_STAT_FIFO, NULL)) < 0)
		return r;
	strcpy(st->st_name, fsipcbuf.statRet.ret_name);
	st->st_size = fsipcbuf.statRet.ret_size;
	st->st_isdir = fsipcbuf.statRet.ret_isdir;
	st->st_isfifo = fsipcbuf.statRet.ret_isfifo;
	return 0;
}

static int
devfifo_close(struct Fd *fd)
{
	fsipcbuf.close_fifo.req_fileid = fd->fd_file.id;
	return fsipc_fifo(FSREQ_CLOSE_FIFO, NULL);
}