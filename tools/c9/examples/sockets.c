#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "c9/proto.h"

enum {
	Msize = 8192,

	Disconnected = 1<<0,
};

typedef struct C9aux C9aux;

struct C9aux {
	C9ctx c;
	int f;
	int flags;
	uint8_t rdbuf[Msize];
	uint8_t wrbuf[Msize];
	uint32_t wroff;
};

static uint8_t *
ctxread(C9ctx *ctx, uint32_t size, int *err)
{
	uint32_t n;
	int r;
	C9aux *a;

	a = ctx->aux;
	*err = 0;
	for(n = 0; n < size; n += r){
		if((r = read(a->f, a->rdbuf+n, size-n)) <= 0){
			if(errno == EINTR)
				continue;
			a->flags |= Disconnected;
			close(a->f);
			return NULL;
		}
	}

	return a->rdbuf;
}

static int
wrsend(C9aux *a)
{
	uint32_t n;
	int w;

	for(n = 0; n < a->wroff; n += w){
		if((w = write(a->f, a->wrbuf+n, a->wroff-n)) <= 0){
			if(errno == EINTR)
				continue;
			if(errno != EPIPE) /* remote end closed */
				perror("write");
			return -1;
		}
	}
	a->wroff = 0;

	return 0;
}

static uint8_t *
ctxbegin(C9ctx *ctx, uint32_t size)
{
	uint8_t *b;
	C9aux *a;

	a = ctx->aux;
	if(a->wroff + size > sizeof(a->wrbuf)){
		if(wrsend(a) != 0 || a->wroff + size > sizeof(a->wrbuf))
			return NULL;
	}
	b = a->wrbuf + a->wroff;
	a->wroff += size;

	return b;
}

static int
ctxend(C9ctx *ctx)
{
	C9aux *a;

	/*
	 * To batch up requests and instead flush them all at once just return 0
	 * here and call wrsend yourself when needed.
	 */
	a = ctx->aux;
	return wrsend(a);
}

static void
ctxerror(C9ctx *ctx, const char *fmt, ...)
{
	va_list ap;

	(void)ctx;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

C9aux *
init9p(int f)
{
	C9aux *c;

	if((c = calloc(1, sizeof(*c))) == NULL){
		close(f);
		return NULL;
	}
	c->f = f;
	c->c.read = ctxread;
	c->c.begin = ctxbegin;
	c->c.end = ctxend;
	c->c.error = ctxerror;
	c->c.aux = c;

	return c;
}
