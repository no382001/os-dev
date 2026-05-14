#pragma once
#include "libc/types.h"

struct C9aux;

typedef struct C9r C9r;
typedef struct C9t C9t;
typedef struct C9stat C9stat;
typedef struct C9ctx C9ctx;
typedef struct C9qid C9qid;
typedef uint32_t C9fid;
typedef uint16_t C9tag;

#define C9nochange (~0)
#define C9nofid ((C9fid)~0)

typedef enum {
  C9read = 0,
  C9write = 1,
  C9rdwr = 2,
  C9exec = 3,
  C9trunc = 0x10,
  C9rclose = 0x40,
  C9excl = 0x1000,
} C9mode;

typedef enum {
  C9permur = 1 << 8,
  C9permuw = 1 << 7,
  C9permux = 1 << 6,
  C9permgr = 1 << 5,
  C9permgw = 1 << 4,
  C9permgx = 1 << 3,
  C9permor = 1 << 2,
  C9permow = 1 << 1,
  C9permox = 1 << 0,
} C9perm;

#define C9permdir 0x80000000
#define C9stdir 0x80000000
#define C9stappend 0x40000000
#define C9stexcl 0x20000000
#define C9sttmp 0x04000000

enum {
  C9maxtags = 64,
  C9maxflush = 8,
  C9maxstr = 0xffff,
  C9minmsize = 4096,
  C9maxpathel = 16,
};

typedef enum {
  C9Einit = -1,
  C9Ever = -2,
  C9Epkt = -3,
  C9Etag = -4,
  C9Ebuf = -5,
  C9Epath = -6,
  C9Eflush = -7,
  C9Esize = -8,
  C9Estr = -9,
} C9error;

typedef enum {
  Tversion = 100,
  Tauth = 102,
  Tattach = 104,
  Tflush = 108,
  Twalk = 110,
  Topen = 112,
  Tcreate = 114,
  Tread = 116,
  Twrite = 118,
  Tclunk = 120,
  Tremove = 122,
  Tstat = 124,
  Twstat = 126,
} C9ttype;

typedef enum {
  Rversion = 101,
  Rauth = 103,
  Rattach = 105,
  Rerror = 107,
  Rflush = 109,
  Rwalk = 111,
  Ropen = 113,
  Rcreate = 115,
  Rread = 117,
  Rwrite = 119,
  Rclunk = 121,
  Rremove = 123,
  Rstat = 125,
  Rwstat = 127,
} C9rtype;

typedef enum {
  C9qtdir = 1 << 7,
  C9qtappend = 1 << 6,
  C9qtexcl = 1 << 5,
  C9qtauth = 1 << 3,
  C9qttmp = 1 << 2,
  C9qtfile = 0,
} C9qt;

struct C9qid {
  uint64_t path;
  uint32_t version;
  C9qt type;
};

struct C9stat {
  C9qid qid;
  uint64_t size;
  char *name;
  char *uid;
  char *gid;
  char *muid;
  uint32_t mode;
  uint32_t atime;
  uint32_t mtime;
};

struct C9r {
  union {
    char *error;
    struct {
      uint8_t *data;
      uint32_t size;
    } read;
    struct {
      uint32_t size;
    } write;
    C9stat stat;
    C9qid qid[C9maxpathel];
  };
  C9rtype type;
  uint32_t iounit;
  int numqid;
  C9tag tag;
};

struct C9t {
  C9ttype type;
  C9tag tag;
  union {
    struct {
      char *uname;
      char *aname;
      C9fid afid;
    } attach;
    struct {
      char *uname;
      char *aname;
      C9fid afid;
    } auth;
    struct {
      char *name;
      uint32_t perm;
      C9mode mode;
    } create;
    struct {
      C9tag oldtag;
    } flush;
    struct {
      C9mode mode;
    } open;
    struct {
      uint64_t offset;
      uint32_t size;
    } read;
    struct {
      char *wname[C9maxpathel + 1];
      C9fid newfid;
    } walk;
    struct {
      uint64_t offset;
      uint8_t *data;
      uint32_t size;
    } write;
    C9stat wstat;
  };
  C9fid fid;
};

enum { C9tagsbits = sizeof(uint32_t) * 8 };

struct C9ctx {
  uint8_t *(*read)(C9ctx *ctx, uint32_t size, int *err)
      __attribute__((nonnull(1, 3)));
  uint8_t *(*begin)(C9ctx *ctx, uint32_t size) __attribute__((nonnull(1)));
  int (*end)(C9ctx *ctx) __attribute__((nonnull(1)));
  void (*r)(C9ctx *ctx, C9r *r) __attribute__((nonnull(1, 2)));
  void (*t)(C9ctx *ctx, C9t *t) __attribute__((nonnull(1, 2)));
  void (*error)(C9ctx *ctx, const char *fmt, ...)
      __attribute__((nonnull(1), format(printf, 2, 3)));
  struct C9aux *aux;
  void *paux;
  uint32_t msize;
#ifndef C9_NO_CLIENT
  uint32_t flush[C9maxflush];
  uint32_t tags[C9maxtags / C9tagsbits];
#endif
  union {
#ifndef C9_NO_CLIENT
    C9tag lowfreetag;
#endif
#ifndef C9_NO_SERVER
    uint16_t svflags;
#endif
  };
};

extern C9error c9parsedir(C9ctx *c, C9stat *stat, uint8_t **data,
                           uint32_t *size) __attribute__((nonnull(1, 2, 3)));

#ifndef C9_NO_CLIENT
extern C9error c9version(C9ctx *c, C9tag *tag, uint32_t msize)
    __attribute__((nonnull(1, 2)));
extern C9error c9auth(C9ctx *c, C9tag *tag, C9fid afid, const char *uname,
                       const char *aname) __attribute__((nonnull(1, 2)));
extern C9error c9flush(C9ctx *c, C9tag *tag, C9tag oldtag)
    __attribute__((nonnull(1, 2)));
extern C9error c9attach(C9ctx *c, C9tag *tag, C9fid fid, C9fid afid,
                         const char *uname, const char *aname)
    __attribute__((nonnull(1, 2)));
extern C9error c9walk(C9ctx *c, C9tag *tag, C9fid fid, C9fid newfid,
                       const char *path[]) __attribute__((nonnull(1, 2, 5)));
extern C9error c9open(C9ctx *c, C9tag *tag, C9fid fid, C9mode mode)
    __attribute__((nonnull(1, 2)));
extern C9error c9create(C9ctx *c, C9tag *tag, C9fid fid, const char *name,
                         uint32_t perm, C9mode mode)
    __attribute__((nonnull(1, 2, 4)));
extern C9error c9read(C9ctx *c, C9tag *tag, C9fid fid, uint64_t offset,
                       uint32_t count) __attribute__((nonnull(1, 2)));
extern C9error c9write(C9ctx *c, C9tag *tag, C9fid fid, uint64_t offset,
                        const void *in, uint32_t count)
    __attribute__((nonnull(1, 2, 5)));
extern C9error c9wrstr(C9ctx *c, C9tag *tag, C9fid fid, const char *s)
    __attribute__((nonnull(1, 2, 4)));
extern C9error c9clunk(C9ctx *c, C9tag *tag, C9fid fid)
    __attribute__((nonnull(1, 2)));
extern C9error c9remove(C9ctx *c, C9tag *tag, C9fid fid)
    __attribute__((nonnull(1, 2)));
extern C9error c9stat(C9ctx *c, C9tag *tag, C9fid fid)
    __attribute__((nonnull(1, 2)));
extern C9error c9wstat(C9ctx *c, C9tag *tag, C9fid fid, const C9stat *s)
    __attribute__((nonnull(1, 2, 4)));
extern C9error c9proc(C9ctx *c) __attribute__((nonnull(1)));
#endif

#ifndef C9_NO_SERVER
extern C9error s9version(C9ctx *c) __attribute__((nonnull(1)));
extern C9error s9auth(C9ctx *c, C9tag tag, const C9qid *aqid)
    __attribute__((nonnull(1, 3)));
extern C9error s9error(C9ctx *c, C9tag tag, const char *err)
    __attribute__((nonnull(1)));
extern C9error s9attach(C9ctx *c, C9tag tag, const C9qid *qid)
    __attribute__((nonnull(1, 3)));
extern C9error s9flush(C9ctx *c, C9tag tag) __attribute__((nonnull(1)));
extern C9error s9walk(C9ctx *c, C9tag tag, C9qid *qids[])
    __attribute__((nonnull(1, 3)));
extern C9error s9open(C9ctx *c, C9tag tag, const C9qid *qid, uint32_t iounit)
    __attribute__((nonnull(1, 3)));
extern C9error s9create(C9ctx *c, C9tag tag, const C9qid *qid, uint32_t iounit)
    __attribute__((nonnull(1, 3)));
extern C9error s9read(C9ctx *c, C9tag tag, const void *data, uint32_t size)
    __attribute__((nonnull(1, 3)));
extern C9error s9readdir(C9ctx *c, C9tag tag, C9stat *st[], int *num,
                          uint64_t *offset, uint32_t size)
    __attribute__((nonnull(1, 3, 4)));
extern C9error s9write(C9ctx *c, C9tag tag, uint32_t size)
    __attribute__((nonnull(1)));
extern C9error s9clunk(C9ctx *c, C9tag tag) __attribute__((nonnull(1)));
extern C9error s9remove(C9ctx *c, C9tag tag) __attribute__((nonnull(1)));
extern C9error s9stat(C9ctx *c, C9tag tag, const C9stat *s)
    __attribute__((nonnull(1, 3)));
extern C9error s9wstat(C9ctx *c, C9tag tag) __attribute__((nonnull(1)));
extern C9error s9proc(C9ctx *c) __attribute__((nonnull(1)));
#endif
