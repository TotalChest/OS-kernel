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

static int devfifo_flush(struct Fd *fd);
static ssize_t devfifo_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfifo_write(struct Fd *fd, const void *buf, size_t n);
static int devfifo_stat(struct Fd *fd, struct Stat *stat);

struct Dev devfifo =
{
	.dev_id =	'o',
	.dev_name =	"fifo",
	.dev_read =	devfifo_read,
	.dev_close =	devfifo_flush,
	.dev_stat =	devfifo_stat,
	.dev_write =	devfifo_write,
};

#define FIFOBUFSIZ 128

struct Fifo {
	off_t fifo_rpos;		// read position
	off_t fifo_wpos;		// write position
	uint8_t fifo_buf[FIFOBUFSIZ];	// data buffer
};

int
mkfifo(const char *path)
{
	int r;
	struct Fd *fd;

	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;

	if ((r = fd_alloc(&fd)) < 0)
		return r;

	strcpy(fsipcbuf.create_fifo.req_path, path);

	if ((r = fsipc_fifo(FSREQ_CREATE_FIFO, NULL)) < 0) {
		return r;
	}

	return 0;
}

static int
devfifo_flush(struct Fd *fd)
{
	fsipcbuf.flush.req_fileid = fd->fd_file.id;
	return fsipc_fifo(FSREQ_FLUSH, NULL);
}

static ssize_t
devfifo_read(struct Fd *fd, void *buf, size_t n)
{
	// Make an FSREQ_READ request to the file system server after
	// filling fsipcbuf.read with the request arguments.  The
	// bytes read will be written back to fsipcbuf by the file
	// system server.
	int r;

	fsipcbuf.read.req_fileid = fd->fd_file.id;
	fsipcbuf.read.req_n = n;
	if ((r = fsipc_fifo(FSREQ_READ, NULL)) < 0)
		return r;
	assert(r <= n);
	assert(r <= PGSIZE);
	memmove(buf, &fsipcbuf, r);
	return r;
}

static ssize_t
devfifo_write(struct Fd *fd, const void *buf, size_t n)
{
	// Make an FSREQ_WRITE request to the file system server.  Be
	// careful: fsipcbuf.write.req_buf is only so large, but
	// remember that write is always allowed to write *fewer*
	// bytes than requested.
	// LAB 10: Your code here
	uint32_t max;

	if (n > (max = PGSIZE - (sizeof(int) + sizeof(size_t))))
		n = max;
	
	fsipcbuf.write.req_fileid = fd->fd_file.id;
	fsipcbuf.write.req_n = n;
	memmove(fsipcbuf.write.req_buf, buf, n);
	
	return fsipc_fifo(FSREQ_WRITE, NULL);
}

static int
devfifo_stat(struct Fd *fd, struct Stat *st)
{
	int r;

	fsipcbuf.stat.req_fileid = fd->fd_file.id;
	if ((r = fsipc_fifo(FSREQ_STAT, NULL)) < 0)
		return r;
	strcpy(st->st_name, fsipcbuf.statRet.ret_name);
	st->st_size = fsipcbuf.statRet.ret_size;
	st->st_isdir = fsipcbuf.statRet.ret_isdir;
	return 0;
}