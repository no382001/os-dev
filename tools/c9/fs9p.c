#include <stdint.h>
#include <string.h>
#include "c9/proto.h"
#include "c9/fs.h"
#include "c9/fs9p.h"

enum
{
	Flinit = 1<<0,
	Flerror = 1<<1,

	F9init = 0,
	F9chdir,
	F9open,
	F9create,
	F9read,
	F9readdir,
	F9write,
	F9seek,
	F9stat,
	F9fstat,
	F9close,
	F9remove,
	F9rename,
};

static const FS9pfd freefd =
{
	.offset = ~0ULL,
};

#define isfree(fd) ((fd).offset == ~0ULL)

static const char *f2s[] =
{
	[F9init] = "init",
	[F9chdir] = "chdir",
	[F9open] = "open",
	[F9create] = "create",
	[F9read] = "read",
	[F9readdir] = "readdir",
	[F9write] = "write",
	[F9seek] = "seek",
	[F9stat] = "stat",
	[F9fstat] = "fstat",
	[F9close] = "close",
	[F9remove] = "remove",
	[F9rename] = "rename",
};

static int
fs9pchdir(FS *fs, const char *path)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9chdir;

	return -1;
}

static int
fs9popen(FS *fs, const char *name, FSmode mode)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9open;

	return -1;
}

static int
fs9pcreate(FS *fs, const char *path, int perm)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9create;

	return -1;
}

static int64_t
fs9pread(FS *fs, int fd, void *buf, uint64_t count)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9read;

	return -1;
}

static int64_t
fs9preaddir(FS *fs, int fd, FSstat *st, int nst)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9readdir;

	return -1;
}

static int64_t
fs9pwrite(FS *fs, int fd, const void *buf, uint64_t count)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9write;

	return -1;
}

static int64_t
fs9pseek(FS *fs, int fd, int64_t offset, int whence)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9seek;

	return -1;
}

static int
fs9pstat(FS *fs, const char *path, FSstat *st)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9stat;

	return -1;
}

static int
fs9pfstat(FS *fs, int fd, FSstat *st)
{
	FS9p *aux = (void*)fs->aux;
	C9tag tag;
	int e;

	aux->f = F9fstat;
	aux->p = st;
	if((e = c9stat(aux->ctx, &tag, fd)) < 0)
		return e;
	while(aux->f != -F9fstat)
		c9proc(aux->ctx);

	return 0;
}

static int
fs9pclose(FS *fs, int fd)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9close;

	return -1;
}

static int
fs9premove(FS *fs, const char *name)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9remove;

	return -1;
}

static int
fs9prename(FS *fs, const char *oldpath, const char *newpath)
{
	FS9p *aux = (void*)fs->aux;

	aux->f = F9rename;

	return -1;
}

static void
fs9pr(C9ctx *ctx, C9r *r)
{
	FSstat *st;
	C9tag tag;
	FS9p *aux;
	FS *fs;

	fs = ctx->paux;
	aux = (void*)fs->aux;

	switch(r->type){
	case Rversion:
		c9attach(ctx, &tag, FS9proot, C9nofid, "none", NULL); /* FIXME those need to be configurable */
		break;

	case Rauth:
		break;

	case Rattach:
		aux->flags |= Flinit;
		break;

	case Rerror:
		fs->error(fs, "%s: %s", f2s[aux->f], r->error);
		aux->flags |= Flerror;
		break;

	case Rflush:
		break;

	case Rwalk:
		break;

	case Ropen:
		break;

	case Rcreate:
		break;

	case Rread:
		break;

	case Rwrite:
		break;

	case Rclunk:
		break;

	case Rremove:
		break;

	case Rstat:
		if(aux->f == F9stat || aux->f == F9fstat){
			st = aux->p;
			st->size = r->stat.size;
			st->name = r->stat.name;
			st->uid = r->stat.uid;
			st->gid = r->stat.gid;
			st->mode = r->stat.mode;
			st->mtime = r->stat.mtime;
			aux->f = -aux->f;
		}
		break;

	case Rwstat:
		break;
	}
}

int
fs9pinit(FS *fs, uint32_t msize)
{
	FS9p *aux = (void*)fs->aux;
	C9error err;
	C9tag tag;
	int i;

	if(fs->error == NULL)
		return -1;
	if(aux == NULL || aux->ctx == NULL || aux->fds == NULL || aux->numfds < 1){
		fs->error(fs, "fs9pinit: invalid aux");
		return -1;
	}
	if(aux->ctx == NULL){
		fs->error(fs, "fs9pinit: invalid ctx");
		return -1;
	}

	fs->chdir = fs9pchdir;
	fs->open = fs9popen;
	fs->create = fs9pcreate;
	fs->read = fs9pread;
	fs->readdir = fs9preaddir;
	fs->write = fs9pwrite;
	fs->seek = fs9pseek;
	fs->stat = fs9pstat;
	fs->fstat = fs9pfstat;
	fs->close = fs9pclose;
	fs->remove = fs9premove;
	fs->rename = fs9prename;

	aux->root.offset = 0;
	for(i = 0; i < aux->numfds; i++)
		aux->fds[i] = freefd;

	aux->ctx->r = fs9pr;
	aux->ctx->paux = fs;
	aux->f = F9init;
	aux->flags = 0;

	if((err = c9version(aux->ctx, &tag, msize)) != 0)
		return err;
	for(;;){
		if((err = c9proc(aux->ctx)) != 0)
			return err;
		if(aux->flags & Flerror)
			return -1;
		if(aux->flags & Flinit)
			break;
	}

	return 0;
}
