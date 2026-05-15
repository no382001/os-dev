#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "c9/proto.h"

#define MAX_CONNS    16
#define MSIZE        8192
#define LINE_BUF     65536
#define DEFAULT_PORT 7564

enum { PROTO_TCP = 0, PROTO_UDP = 1 };

typedef struct conn_t conn_t;
struct C9aux { conn_t *conn; };

struct conn_t {
	int       id, used, fd, proto;
	struct C9aux aux;
	C9ctx     ctx;
	uint8_t   rdbuf[MSIZE];
	uint8_t   wrbuf[MSIZE];
	uint32_t  wroff;
	/* UDP: position within current buffered datagram */
	uint32_t  rdpos, rdlen;
	/* per-operation result (populated by on_r) */
	int       done, err;
	C9qid     qid;
	C9stat    rstat;
	uint8_t  *rdata;
	uint32_t  rcount, wcount;
	C9fid     next_fid;
};

static conn_t  g_conns[MAX_CONNS];
static FILE   *g_out;

/* --- transport callbacks ------------------------------------------- */

static uint8_t *
ctx_read(C9ctx *ctx, uint32_t size, int *err)
{
	conn_t *c = ctx->aux->conn;
	*err = 0;

	if (c->proto == PROTO_UDP) {
		if (c->rdpos >= c->rdlen) {
			int r = (int)recv(c->fd, c->rdbuf, MSIZE, 0);
			if (r <= 0) { *err = 1; return NULL; }
			c->rdlen = (uint32_t)r;
			c->rdpos = 0;
		}
		if (c->rdpos + size > c->rdlen) { *err = 1; return NULL; }
		uint8_t *p = c->rdbuf + c->rdpos;
		c->rdpos += size;
		return p;
	}

	uint32_t n = 0;
	int r;
	while (n < size) {
		r = (int)read(c->fd, c->rdbuf + n, size - n);
		if (r <= 0) { *err = 1; return NULL; }
		n += (uint32_t)r;
	}
	return c->rdbuf;
}

static uint8_t *
ctx_begin(C9ctx *ctx, uint32_t size)
{
	conn_t *c = ctx->aux->conn;
	if (c->wroff + size > MSIZE) return NULL;
	uint8_t *b = c->wrbuf + c->wroff;
	c->wroff += size;
	return b;
}

static int
ctx_end(C9ctx *ctx)
{
	conn_t *c = ctx->aux->conn;
	if (c->proto == PROTO_UDP) {
		int w = (int)send(c->fd, c->wrbuf, c->wroff, 0);
		c->wroff = 0;
		return (w < 0) ? -1 : 0;
	}
	uint32_t n = 0;
	int w;
	while (n < c->wroff) {
		w = (int)write(c->fd, c->wrbuf + n, c->wroff - n);
		if (w <= 0) { c->wroff = 0; return -1; }
		n += (uint32_t)w;
	}
	c->wroff = 0;
	return 0;
}

static void
ctx_error(C9ctx *ctx, const char *fmt, ...)
{
	(void)ctx;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static void
on_r(C9ctx *ctx, C9r *r)
{
	conn_t *c = ctx->aux->conn;
	c->done = 1;
	switch (r->type) {
	case Rversion: break;
	case Rattach: case Ropen: case Rcreate: c->qid = r->qid[0]; break;
	case Rwalk:   if (r->numqid > 0) c->qid = r->qid[r->numqid - 1]; break;
	case Rstat:   c->rstat = r->stat; break;
	case Rread:   c->rdata = r->read.data; c->rcount = r->read.size; break;
	case Rwrite:  c->wcount = r->write.size; break;
	case Rerror:  fprintf(stderr, "Rerror: %s\n", r->error); c->err = 1; break;
	default: break;
	}
}

static int
roundtrip(conn_t *c)
{
	c->done = c->err = 0;
	while (!c->done && !c->err)
		if (c9proc(&c->ctx) != 0) return -1;
	return c->err ? -1 : 0;
}

/* --- output helpers ------------------------------------------------ */

static void
atom_write(FILE *f, const char *s, int len)
{
	fputc('\'', f);
	for (int i = 0; i < len; i++) {
		unsigned char ch = (unsigned char)s[i];
		if      (ch == '\'') { fputc('\\', f); fputc('\'', f); }
		else if (ch == '\\') { fputc('\\', f); fputc('\\', f); }
		else if (ch == '\n') { fputc('\\', f); fputc('n',  f); }
		else if (ch == '\r') { fputc('\\', f); fputc('r',  f); }
		else if (ch == '\t') { fputc('\\', f); fputc('t',  f); }
		else                   fputc(ch, f);
	}
	fputc('\'', f);
}

#define ATOM(s)    atom_write(g_out, (s),           (int)strlen(s))
#define ATOMN(s,n) atom_write(g_out, (const char*)(s), (int)(n))

/* --- connection management ----------------------------------------- */

static conn_t *
find_conn(int id)
{
	for (int i = 0; i < MAX_CONNS; i++)
		if (g_conns[i].used && g_conns[i].id == id) return &g_conns[i];
	return NULL;
}

static void
free_conn(conn_t *c)
{
	close(c->fd);
	memset(c, 0, sizeof(*c));
}

/* --- path walk ----------------------------------------------------- */

static int
do_walk(conn_t *c, C9fid from, C9fid to, const char *path)
{
	char buf[1024];
	strncpy(buf, path, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	const char *parts[C9maxpathel + 1];
	int np = 0;
	char *p = buf;
	while (*p == '/') p++;
	while (*p && np < C9maxpathel) {
		parts[np++] = p;
		while (*p && *p != '/') p++;
		if (*p) { *p++ = '\0'; while (*p == '/') p++; }
	}
	parts[np] = NULL;

	C9tag tag;
	if (c9walk(&c->ctx, &tag, from, to, parts) != 0) return -1;
	return roundtrip(c);
}

/* --- commands ------------------------------------------------------ */

/* parse "tcp!host!port" or "udp!host!port" into proto/host/port */
static int
parse_dialstr(const char *s, int *proto, char *host, size_t hsz, char *port_str, size_t psz)
{
	const char *p;
	if (strncmp(s, "udp!", 4) == 0)      { *proto = PROTO_UDP; p = s + 4; }
	else if (strncmp(s, "tcp!", 4) == 0) { *proto = PROTO_TCP; p = s + 4; }
	else                                  { *proto = PROTO_TCP; p = s;     }

	const char *bang = strrchr(p, '!');
	if (!bang) return -1;

	size_t hlen = (size_t)(bang - p);
	if (hlen == 0 || hlen >= hsz) return -1;
	strncpy(host, p, hlen); host[hlen] = '\0';
	strncpy(port_str, bang + 1, psz - 1); port_str[psz - 1] = '\0';
	return 0;
}

static void
cmd_connect(int id, const char *dialstr)
{
	if (find_conn(id)) {
		fprintf(g_out, "error(%d,'already connected').\n", id);
		fflush(g_out); return;
	}
	conn_t *c = NULL;
	for (int i = 0; i < MAX_CONNS; i++)
		if (!g_conns[i].used) { c = &g_conns[i]; break; }
	if (!c) {
		fprintf(g_out, "error(%d,'too many connections').\n", id);
		fflush(g_out); return;
	}

	int proto;
	char host[256], port_str[16];
	if (parse_dialstr(dialstr, &proto, host, sizeof(host), port_str, sizeof(port_str)) < 0) {
		fprintf(g_out, "error(%d,'bad dial string').\n", id);
		fflush(g_out); return;
	}

	struct addrinfo hint = {
		.ai_flags    = AI_ADDRCONFIG,
		.ai_family   = AF_UNSPEC,
		.ai_socktype = (proto == PROTO_UDP) ? SOCK_DGRAM  : SOCK_STREAM,
		.ai_protocol = (proto == PROTO_UDP) ? IPPROTO_UDP : IPPROTO_TCP,
	};
	struct addrinfo *r, *a;
	int e = getaddrinfo(host, port_str, &hint, &r);
	if (e != 0) {
		fprintf(g_out, "error(%d,", id); ATOM(gai_strerror(e));
		fprintf(g_out, ").\n"); fflush(g_out); return;
	}

	int fd = -1;
	for (a = r; a; a = a->ai_next) {
		fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
		if (fd < 0) continue;
		if (connect(fd, a->ai_addr, a->ai_addrlen) == 0) break;
		close(fd); fd = -1;
	}
	freeaddrinfo(r);

	if (fd < 0) {
		fprintf(g_out, "error(%d,'connect failed').\n", id);
		fflush(g_out); return;
	}

	memset(c, 0, sizeof(*c));
	c->id = id; c->used = 1; c->fd = fd; c->proto = proto; c->next_fid = 2;
	c->aux.conn  = c;
	c->ctx.read  = ctx_read;
	c->ctx.begin = ctx_begin;
	c->ctx.end   = ctx_end;
	c->ctx.error = ctx_error;
	c->ctx.r     = on_r;
	c->ctx.aux   = &c->aux;

	C9tag tag;
	if (c9version(&c->ctx, &tag, MSIZE) != 0 || roundtrip(c) < 0) {
		free_conn(c);
		fprintf(g_out, "error(%d,'version failed').\n", id);
		fflush(g_out); return;
	}
	if (c9attach(&c->ctx, &tag, 1, C9nofid, "none", "") != 0 || roundtrip(c) < 0) {
		free_conn(c);
		fprintf(g_out, "error(%d,'attach failed').\n", id);
		fflush(g_out); return;
	}
	fprintf(g_out, "connected(%d).\n", id);
	fflush(g_out);
}

static void
cmd_disconnect(int id)
{
	conn_t *c = find_conn(id);
	if (!c) {
		fprintf(g_out, "error(%d,'not connected').\n", id);
		fflush(g_out); return;
	}
	C9tag tag;
	c9clunk(&c->ctx, &tag, 1);
	roundtrip(c);
	free_conn(c);
	fprintf(g_out, "disconnected(%d,ok).\n", id);
	fflush(g_out);
}

static void
cmd_read(int id, const char *path)
{
	conn_t *c = find_conn(id);
	if (!c) {
		fprintf(g_out, "error(%d,'not connected').\n", id);
		fflush(g_out); return;
	}
	C9fid fid = c->next_fid++;
	C9tag tag;

	if (do_walk(c, 1, fid, path) < 0) {
		fprintf(g_out, "error(%d,'walk failed').\n", id);
		fflush(g_out); return;
	}
	if (c9open(&c->ctx, &tag, fid, C9read) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'open failed').\n", id);
		fflush(g_out); return;
	}
	if (c9read(&c->ctx, &tag, fid, 0, c->ctx.msize) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'read failed').\n", id);
		fflush(g_out); return;
	}
	/* copy data before clunk overwrites rdbuf */
	uint8_t  tmp[MSIZE];
	uint32_t len = c->rcount;
	memcpy(tmp, c->rdata, len);

	c9clunk(&c->ctx, &tag, fid);
	roundtrip(c);

	fprintf(g_out, "data(%d,", id); ATOM(path); fputc(',', g_out);
	ATOMN(tmp, len);
	fprintf(g_out, ").\n");
	fflush(g_out);
}

static void
cmd_write(int id, const char *path, const char *data)
{
	conn_t *c = find_conn(id);
	if (!c) {
		fprintf(g_out, "error(%d,'not connected').\n", id);
		fflush(g_out); return;
	}
	C9fid fid = c->next_fid++;
	C9tag tag;

	if (do_walk(c, 1, fid, path) < 0) {
		fprintf(g_out, "error(%d,'walk failed').\n", id);
		fflush(g_out); return;
	}
	if (c9open(&c->ctx, &tag, fid, (C9mode)(C9write | C9trunc)) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'open failed').\n", id);
		fflush(g_out); return;
	}
	uint32_t dlen = (uint32_t)strlen(data);
	if (c9write(&c->ctx, &tag, fid, 0, data, dlen) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'write failed').\n", id);
		fflush(g_out); return;
	}
	uint32_t written = c->wcount;
	c9clunk(&c->ctx, &tag, fid);
	roundtrip(c);

	fprintf(g_out, "written(%d,", id); ATOM(path);
	fprintf(g_out, ",%u).\n", written);
	fflush(g_out);
}

static void
cmd_ls(int id, const char *path)
{
	conn_t *c = find_conn(id);
	if (!c) {
		fprintf(g_out, "error(%d,'not connected').\n", id);
		fflush(g_out); return;
	}
	C9fid fid = c->next_fid++;
	C9tag tag;

	if (do_walk(c, 1, fid, path) < 0) {
		fprintf(g_out, "error(%d,'walk failed').\n", id);
		fflush(g_out); return;
	}
	if (c9open(&c->ctx, &tag, fid, C9read) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'open failed').\n", id);
		fflush(g_out); return;
	}
	if (c9read(&c->ctx, &tag, fid, 0, c->ctx.msize) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'read failed').\n", id);
		fflush(g_out); return;
	}
	/* parse and send all entries before clunking (strings point into rdbuf) */
	uint8_t  *p   = c->rdata;
	uint32_t  rem = c->rcount;
	C9stat    st;
	while (rem > 0) {
		if (c9parsedir(&c->ctx, &st, &p, &rem) != 0) break;
		fprintf(g_out, "entry(%d,", id); ATOM(path); fputc(',', g_out);
		ATOM(st.name);
		fprintf(g_out, ",%llu,%u).\n",
			(unsigned long long)st.size, st.mode);
		fflush(g_out);
	}
	c9clunk(&c->ctx, &tag, fid);
	roundtrip(c);
	fprintf(g_out, "ls_done(%d,", id); ATOM(path);
	fprintf(g_out, ").\n");
	fflush(g_out);
}

static void
cmd_stat(int id, const char *path)
{
	conn_t *c = find_conn(id);
	if (!c) {
		fprintf(g_out, "error(%d,'not connected').\n", id);
		fflush(g_out); return;
	}
	C9fid fid = c->next_fid++;
	C9tag tag;

	if (do_walk(c, 1, fid, path) < 0) {
		fprintf(g_out, "error(%d,'walk failed').\n", id);
		fflush(g_out); return;
	}
	if (c9stat(&c->ctx, &tag, fid) != 0 || roundtrip(c) < 0) {
		c9clunk(&c->ctx, &tag, fid); roundtrip(c);
		fprintf(g_out, "error(%d,'stat failed').\n", id);
		fflush(g_out); return;
	}
	/* use stat strings before clunk overwrites rdbuf */
	fprintf(g_out, "stat(%d,", id); ATOM(path); fputc(',', g_out);
	ATOM(c->rstat.name);
	fprintf(g_out, ",%llu,%u).\n",
		(unsigned long long)c->rstat.size, c->rstat.mode);
	fflush(g_out);
	c9clunk(&c->ctx, &tag, fid);
	roundtrip(c);
}

/* --- dispatch ------------------------------------------------------ */

static int
split_tabs(char *line, char **parts, int maxparts)
{
	int n = 0;
	char *p = line;
	while (n < maxparts - 1) {
		parts[n++] = p;
		char *t = strchr(p, '\t');
		if (!t) break;
		*t = '\0';
		p = t + 1;
	}
	if (n < maxparts) parts[n++] = p;
	return n;
}

typedef void (*cmd_fn)(char **a);
typedef struct { const char *name; int min_args; cmd_fn fn; } cmd_entry;

static void d_connect(char **a)    { cmd_connect(atoi(a[0]), a[1]); }
static void d_disconnect(char **a) { cmd_disconnect(atoi(a[0])); }
static void d_read(char **a)       { cmd_read(atoi(a[0]), a[1]); }
static void d_write(char **a)      { cmd_write(atoi(a[0]), a[1], a[2] ? a[2] : ""); }
static void d_ls(char **a)         { cmd_ls(atoi(a[0]), a[1]); }
static void d_stat(char **a)       { cmd_stat(atoi(a[0]), a[1]); }

static const cmd_entry cmd_table[] = {
	{"connect",    2, d_connect},
	{"disconnect", 1, d_disconnect},
	{"read",       2, d_read},
	{"write",      2, d_write},
	{"ls",         2, d_ls},
	{"stat",       2, d_stat},
};
#define NTABLE (int)(sizeof(cmd_table) / sizeof(cmd_table[0]))

static void
dispatch_line(char *line)
{
	char *parts[7] = {0};
	int n = split_tabs(line, parts, 7);
	if (n == 0) return;
	char *cmd = parts[0];
	char **a = parts + 1;
	int nargs = n - 1;
	for (int i = 0; i < NTABLE; i++) {
		if (strcmp(cmd, cmd_table[i].name) == 0) {
			if (nargs < cmd_table[i].min_args) return;
			cmd_table[i].fn(a);
			return;
		}
	}
}

/* --- main ---------------------------------------------------------- */

int
main(int argc, char **argv)
{
	int port = DEFAULT_PORT;
	if (argc > 1) port = atoi(argv[1]);

	int srv = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in addr = {0};
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons((uint16_t)port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind"); return 1;
	}
	listen(srv, 1);
	fprintf(stderr, "c9_bridge: listening on :%d\n", port);

	for (;;) {
		int fd = accept(srv, NULL, NULL);
		if (fd < 0) continue;
		fprintf(stderr, "c9_bridge: prolog connected\n");

		g_out     = fdopen(dup(fd), "w");
		FILE *in  = fdopen(fd, "r");

		char line[LINE_BUF];
		while (fgets(line, sizeof(line), in)) {
			int len = (int)strlen(line);
			while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
				line[--len] = '\0';
			if (len > 0) dispatch_line(line);
		}

		fprintf(stderr, "c9_bridge: prolog disconnected\n");
		fclose(in);
		if (g_out) { fclose(g_out); g_out = NULL; }
		for (int i = 0; i < MAX_CONNS; i++)
			if (g_conns[i].used) free_conn(&g_conns[i]);
	}
	close(srv);
	return 0;
}
