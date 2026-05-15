typedef struct FS FS;
typedef struct FSstat FSstat;

typedef enum
{
	FS_OREAD,
	FS_OWRITE,
	FS_ORDWR,
	FS_OEXEC,
	FS_OTRUNC = 0x10,
	FS_ORCLOSE = 0x40,
	FS_OEXCL = 0x1000,
	FS_DIR = 0x80000000U, /* open/create: it's supposed to be a directory. */
}FSmode;

struct FSstat
{
	uint64_t size; /* Size of the file (in bytes). */
	char *name;  /* Name of the file. */
	char *uid;   /* Owner of the file. */
	char *gid;   /* Group of the file. */
	uint32_t mode;   /* Permissions. See C9st* and C9perm. */
	uint64_t mtime;  /* Last modification time. Nanoseconds since epoch. */
};

struct FSaux;

struct FS
{
	/* Callback for error messages. */
	void (*error)(FS *fs, const char *fmt, ...) __attribute__((nonnull(1), format(printf, 2, 3)));

	/* Do not set these. */
	int (*chdir)(FS *fs, const char *path) __attribute__((nonnull(1, 2)));
	int (*open)(FS *fs, const char *name, FSmode mode) __attribute__((nonnull(1, 2)));
	int (*create)(FS *fs, const char *path, int perm) __attribute__((nonnull(1, 2))); 
	int64_t (*read)(FS *fs, int fd, void *buf, uint64_t count) __attribute__((nonnull(1, 3)));
	int64_t (*readdir)(FS *fs, int fd, FSstat *st, int nst);
	int64_t (*write)(FS *fs, int fd, const void *buf, uint64_t count) __attribute__((nonnull(1, 3)));
	int64_t (*seek)(FS *fs, int fd, int64_t offset, int whence) __attribute__((nonnull(1)));
	int (*stat)(FS *fs, const char *path, FSstat *st) __attribute__((nonnull(1, 2, 3)));
	int (*fstat)(FS *fs, int fd, FSstat *st) __attribute__((nonnull(1, 3)));
	int (*close)(FS *fs, int fd) __attribute__((nonnull(1)));
	int (*remove)(FS *fs, const char *name) __attribute__((nonnull(1, 2)));
	int (*rename)(FS *fs, const char *oldpath, const char *newpath) __attribute__((nonnull(1, 2, 3)));

	struct FSaux *aux;
};
