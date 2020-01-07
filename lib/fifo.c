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

static ssize_t
devfifo_read(struct Fd *fd, void *buf, size_t n)
{
	int r;
	int c = 0;
	
	while(1){

		fsipcbuf.read_fifo.req_fileid = fd->fd_file.id;
		fsipcbuf.read_fifo.req_n = n - c;

		if ((r = fsipc_fifo(FSREQ_READ_FIFO, NULL)) < 0){
			if (r == -E_FIFO){
				memmove(buf + c, fsipcbuf.readRet.ret_buf, fsipcbuf.readRet.ret_n);
				c += fsipcbuf.readRet.ret_n;
				if (debug)
					cprintf("read plan\n");
				sys_yield();
				continue;
			}

			return r;
		}

		memmove(buf + c, fsipcbuf.readRet.ret_buf, r);

		c += r;

		break;
	}

	return c;

}

static ssize_t
devfifo_write(struct Fd *fd, const void *buf, size_t n)
{
	uint32_t max;
	int r;
	int c = 0;

	if (n > (max = PGSIZE - (sizeof(int) + sizeof(size_t))))
		n = max;

	while(1){

		fsipcbuf.write_fifo.req_fileid = fd->fd_file.id;
		fsipcbuf.write_fifo.req_n = n - c;
		memmove(fsipcbuf.write_fifo.req_buf, buf + c, n - c);

		if ((r = fsipc_fifo(FSREQ_WRITE_FIFO, NULL)) < 0){
			if (r == -E_FIFO){
				c += fsipcbuf.writeRet.ret_n;
				if (debug)
					cprintf("write plan\n");
				sys_yield();
				continue;
			}

			return r;
		}

		c += r;

		break;
	}

	return c;
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