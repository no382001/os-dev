#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
	/* receive: buffer one full UDP datagram, slice it */
	uint8_t rdbuf[Msize];
	uint32_t rdlen;
	uint32_t rdpos;
	/* send: accumulate into one datagram, flush on ctxend */
	uint8_t wrbuf[Msize];
	uint32_t wroff;
};

static uint8_t *
ctxread(C9ctx *ctx, uint32_t size, int *err)
{
	C9aux *a = ctx->aux;
	*err = 0;

	/* refill when buffer is exhausted */
	if (a->rdpos >= a->rdlen) {
		int r = recv(a->f, a->rdbuf, sizeof(a->rdbuf), 0);
		if (r <= 0) {
			a->flags |= Disconnected;
			*err = 1;
			return NULL;
		}
		a->rdlen = (uint32_t)r;
		a->rdpos = 0;
	}

	if (a->rdpos + size > a->rdlen) {
		fprintf(stderr, "udp: short datagram (have %u, want %u)\n",
			a->rdlen - a->rdpos, size);
		*err = 1;
		return NULL;
	}

	uint8_t *p = a->rdbuf + a->rdpos;
	a->rdpos += size;
	return p;
}

static uint8_t *
ctxbegin(C9ctx *ctx, uint32_t size)
{
	C9aux *a = ctx->aux;
	if (a->wroff + size > sizeof(a->wrbuf))
		return NULL;
	uint8_t *b = a->wrbuf + a->wroff;
	a->wroff += size;
	return b;
}

static int
ctxend(C9ctx *ctx)
{
	C9aux *a = ctx->aux;
	int w = send(a->f, a->wrbuf, a->wroff, 0);
	a->wroff = 0;
	if (w < 0) {
		perror("send");
		return -1;
	}
	return 0;
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
	if ((c = calloc(1, sizeof(*c))) == NULL) {
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
