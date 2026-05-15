#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "c9/proto.h"
#include "c9/fs.h"
#include "c9/fs9p.h"

int dial(const char *s);
C9ctx *init9p(int f);

static FS9pfd fds[8];

static void
fserror(FS *fs, const char *fmt, ...)
{
	va_list ap;

	(void)fs;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

int
main(int argc, char **argv)
{
	C9ctx *ctx;
	FSstat st;
	FS fs;
	FS9p fs9 = {
		.fds = fds,
		.numfds = sizeof(fds)/sizeof(fds[0]),
	};
	int f;

	(void)argc; (void)argv;

	fs.error = fserror;
	fs.aux = (void*)&fs9;
	if((f = dial("tcp!ftrv.se!564")) < 0)
		return 1;
	ctx = init9p(f);
	fs9.ctx = ctx;
	if(fs9pinit(&fs, 8192) != 0)
		return 1;
	if(fs.fstat(&fs, FS9proot, &st) != 0)
		return 1;
	printf("%s %d %s %o\n", st.name, (int)st.size, st.uid, st.mode&0777);

	return 0;
}
