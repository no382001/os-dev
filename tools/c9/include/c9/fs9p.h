enum
{
	FS9proot = 3, /* '/' file descriptor. */
};

typedef struct FS9p FS9p;
typedef struct FS9pfd FS9pfd;

struct FS9pfd
{
	uint64_t offset;
};

struct FS9p
{
	/* The following three fields need to be set before calling fs9pinit. */

	C9ctx *ctx; /* Set to a full set up context, except "r" and "t" fields. */
	FS9pfd *fds; /* Point at the allocated fds array. */
	int numfds; /* Set to the number of entries available in fds array. */
	void *aux; /* Optional, user-supplied aux value. */

	/* Private, do not touch. */
	FS9pfd root;
	FS9pfd cwd;
	int f;
	void *p;
	int flags;
};

extern int fs9pinit(FS *fs, uint32_t msize) __attribute__((nonnull(1)));
