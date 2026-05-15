/*
 * udptest: walk a 9P2000 server over UDP and print the /fd directory.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "c9/proto.h"

int dial(const char *s);
void *init9p(int f);

typedef struct C9aux C9aux;
struct C9aux { C9ctx c; int f; };  /* completed by udp_sockets.c */

static C9ctx *gctx;

static struct {
	int done;
	int err;
	C9qid qid;
	C9stat stat;
	const uint8_t *rdata;
	uint32_t rcount;
} g;

static void
on_r(C9ctx *ctx, C9r *r)
{
	(void)ctx;
	g.done = 1;
	switch (r->type) {
	case Rversion:
		/* msize updated in ctx->msize by c9proc */
		break;
	case Rattach:
	case Ropen:
		g.qid = r->qid[0];
		break;
	case Rwalk:
		if (r->numqid > 0)
			g.qid = r->qid[r->numqid - 1];
		break;
	case Rstat:
		g.stat = r->stat;
		break;
	case Rread:
		g.rdata  = r->read.data;
		g.rcount = r->read.size;
		break;
	case Rerror:
		fprintf(stderr, "Rerror: %s\n", r->error);
		g.err = 1;
		break;
	default:
		break;
	}
}

static void
on_err(C9ctx *ctx, const char *fmt, ...)
{
	va_list ap;
	(void)ctx;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	g.err = 1;
}

static int
roundtrip(C9ctx *ctx)
{
	g.done = 0;
	g.err  = 0;
	while (!g.done && !g.err) {
		if (c9proc(ctx) != 0)
			return -1;
	}
	return g.err ? -1 : 0;
}

int
main(int argc, char **argv)
{
	C9ctx *ctx;
	C9tag tag;
	int f;
	char dial_str[256];
	const char *wnames[2];

	if (argc < 3) {
		fprintf(stderr, "usage: %s host port\n", argv[0]);
		return 1;
	}
	snprintf(dial_str, sizeof(dial_str), "udp!%s!%s", argv[1], argv[2]);

	if ((f = dial(dial_str)) < 0)
		return 1;
	ctx = (C9ctx *)init9p(f);
	ctx->r     = on_r;
	ctx->error = on_err;

	/* Tversion */
	if (c9version(ctx, &tag, 4096) != 0) { fputs("version send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)              { fputs("Rversion failed\n", stderr); return 1; }
	printf("Rversion: msize=%u\n", ctx->msize);

	/* Tattach fid=1 */
	if (c9attach(ctx, &tag, 1, C9nofid, "none", "") != 0) { fputs("attach send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)                               { fputs("Rattach failed\n", stderr); return 1; }
	printf("Rattach: qid type=%x path=%llx\n",
		g.qid.type, (unsigned long long)g.qid.path);

	/* Tstat fid=1 → root */
	if (c9stat(ctx, &tag, 1) != 0) { fputs("stat send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)        { fputs("Rstat failed\n", stderr); return 1; }
	printf("Rstat /: name=%s size=%llu mode=%04o uid=%s\n",
		g.stat.name, (unsigned long long)g.stat.size,
		g.stat.mode & 0777, g.stat.uid);

	/* Twalk fid=1 → newfid=2, walk to /fd */
	wnames[0] = "fd";
	wnames[1] = NULL;
	if (c9walk(ctx, &tag, 1, 2, wnames) != 0) { fputs("walk send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)                    { fputs("Rwalk failed\n", stderr); return 1; }
	printf("Rwalk /fd: qid type=%x path=%llx\n",
		g.qid.type, (unsigned long long)g.qid.path);

	/* Topen fid=2 mode=read */
	if (c9open(ctx, &tag, 2, C9read) != 0) { fputs("open send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)                { fputs("Ropen failed\n", stderr); return 1; }
	printf("Ropen /fd: iounit will be used implicitly\n");

	/* Tread fid=2 offset=0 count=4096 */
	if (c9read(ctx, &tag, 2, 0, 4096) != 0) { fputs("read send\n", stderr); return 1; }
	if (roundtrip(ctx) < 0)                  { fputs("Rread failed\n", stderr); return 1; }
	printf("Rread /fd: %u bytes\n\n", g.rcount);

	/* parse directory entries with c9parsedir */
	{
		uint8_t *p = (uint8_t *)g.rdata;
		uint32_t rem = g.rcount;
		C9stat st;
		printf("/fd:\n");
		while (rem > 0) {
			if (c9parsedir(ctx, &st, &p, &rem) != 0)
				break;
			printf("  %c  %8llu  %s\n",
				(st.mode & C9stdir) ? 'd' : '-',
				(unsigned long long)st.size, st.name);
		}
	}

	return 0;
}
