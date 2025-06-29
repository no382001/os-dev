#pragma once
#include "libc/types.h"

#define NINEP_PORT 9999

typedef uint32_t nine_fid_t;
typedef uint16_t nine_tag_t;

typedef enum {
  C9read = 0,
  C9write = 1,
  C9rdwr = 2,
  C9exec = 3,
  C9trunc = 0x10,
  C9rclose = 0x40,
  C9excl = 0x1000,
} nine_mode;

// request
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
  Twstat = 126
} nine_t_type_t;

// response
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
  Rwstat = 127
} nine_r_type_t;

enum {
  C9tagsbits = sizeof(uint32_t) * 8,
};

// limits
enum {
  C9maxtags = 64,    /* Maximal number of outstanding requests. [1-65535] */
  C9maxflush = 8,    /* Maximal number of outstanding flushes. [1-65535] */
  C9maxstr = 0xffff, /* Maximal string length. [1-65535] */
  C9minmsize = 4096, /* Minimal sane msize. [4096-...] */
  C9maxpathel = 16,  /* Maximal number of elements in a path. Do not change. */
};

/* Unique file id type. */
typedef enum {
  C9qtdir = 1 << 7,
  C9qtappend = 1 << 6,
  C9qtexcl = 1 << 5,
  C9qtauth = 1 << 3,
  C9qttmp = 1 << 2,
  C9qtfile = 0
} nine_qt;

typedef struct nine_qid nine_qid;
struct nine_qid {
  uint64_t path;
  uint32_t version;
  nine_qt type;
};

enum {
  Msize = 8192,

  Disconnected = 1 << 0,
};

typedef enum {
  C9Einit = -1,  /* Initialization failed. */
  C9Ever = -2,   /* Protocol version doesn't match. */
  C9Epkt = -3,   /* Incoming packet error. */
  C9Etag = -4,   /* No free tags or bad tag. */
  C9Ebuf = -5,   /* No buffer space enough for a message. */
  C9Epath = -6,  /* Path is too long or just invalid. */
  C9Eflush = -7, /* Limit of outstanding flushes reached. */
  C9Esize = -8,  /* Can't fit data in one message. */
  C9Estr = -9    /* Bad string. */
} nine_error;

/*
 * File stats. Version and muid are ignored on wstat. Dmdir bit
 * change in mode won't work on wstat. Set any integer field to
 * C9nochange to keep it unchanged on wstat. Set any string to NULL to
 * keep it unchanged. Strings can be empty (""), but never NULL after
 * stat call.
 */
typedef struct nine_stat nine_stat;
struct nine_stat {
  nine_qid qid;   /* Same as qid[0]. */
  uint64_t size;  /* Size of the file (in bytes). */
  char *name;     /* Name of the file. */
  char *uid;      /* Owner of the file. */
  char *gid;      /* Group of the file. */
  char *muid;     /* The user who modified the file last. */
  uint32_t mode;  /* Permissions. See C9st* and C9perm. */
  uint32_t atime; /* Last access time. */
  uint32_t mtime; /* Last modification time. */
};

// request data
typedef struct nine_transmit nine_transmit;
struct nine_transmit {
  nine_t_type_t type;
  nine_tag_t tag;
  union {
    struct {
      char *uname;
      char *aname;
      nine_fid_t afid;
    } attach;

    struct {
      char *uname;
      char *aname;
      nine_fid_t afid;
    } auth;

    struct {
      char *name;
      uint32_t perm;
      nine_mode mode;
    } create;

    struct {
      nine_tag_t oldtag;
    } flush;

    struct {
      nine_mode mode;
    } open;

    struct {
      uint64_t offset;
      uint32_t size;
    } read;

    struct {
      char *wname[C9maxpathel + 1]; /* wname[16] is always NULL */
      nine_fid_t newfid;
    } walk;

    struct {
      uint64_t offset;
      uint8_t *data;
      uint32_t size;
    } write;

    nine_stat wstat;
  };
  nine_fid_t fid;
};

// response data
typedef struct nine_response nine_response;
struct nine_response {
  union {
    char *error;

    struct {
      uint8_t *data;
      uint32_t size;
    } read;

    struct {
      uint32_t size;
    } write;

    /* File stats (only valid if type is Rstat). */
    nine_stat stat;

    /*
     * Qid(s). qid[0] is valid for auth/attach/create/stat/open.
     * More ids may be a result of a walk, see numqid.
     */
    nine_qid qid[C9maxpathel];
  };
  nine_r_type_t type; /* Response type. */

  /*
   * If not zero, is the maximum number of bytes that are guaranteed
   * to be read or written atomically, without breaking into multiple
   * messages.
   */
  uint32_t iounit;

  int numqid;     /* Number of valid unique ids in qid array. */
  nine_tag_t tag; /* Tag number. */
};

typedef struct nine_aux nine_aux;

typedef struct nine_ctx nine_ctx;
struct nine_ctx {
  /*
   * Should return a pointer to the data (exactly 'size' bytes) read.
   * Set 'err' to non-zero and return NULL in case of error.
   * 'err' set to zero (no error) should be used to return from c9process
   * early (timeout on read to do non-blocking operations, for example).
   */
  uint8_t *(*read)(nine_ctx *ctx, uint32_t size, int *err)
      __attribute__((nonnull(1, 3)));

  /* Should return a buffer to store 'size' bytes. Nil means no memory. */
  uint8_t *(*begin)(nine_ctx *ctx, uint32_t size) __attribute__((nonnull(1)));

  /*
   * Marks the end of a message. Callback may decide if any accumulated
   * messages should be sent to the server/client.
   */
  int (*end)(nine_ctx *ctx) __attribute__((nonnull(1)));

  /* Callback called every time a new R-message is received. */
  void (*r)(nine_ctx *ctx, nine_response *r) __attribute__((nonnull(1, 2)));

  /* Callback called every time a new T-message is received. */
  void (*t)(nine_ctx *ctx, nine_transmit *t) __attribute__((nonnull(1, 2)));

  /* Callback for error messages. */
  void (*error)(nine_ctx *ctx, const char *fmt, ...)
      __attribute__((nonnull(1), format(printf, 2, 3)));

  /* Auxiliary data, can be used by any of above callbacks. */
  nine_aux *aux;

  /* private stuff */
  void *paux;
  uint32_t msize;

  // client
  uint32_t flush[C9maxflush];
  uint32_t tags[C9maxtags / C9tagsbits];
  union {
    // client
    nine_tag_t lowfreetag;
    // server
    uint16_t svflags;
  };
};

struct nine_aux {
  nine_ctx c;
  int f;
  int flags;
  uint8_t rdbuf[Msize];
  uint8_t wrbuf[Msize];
  uint32_t wroff;
};