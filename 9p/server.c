#include "bits.h"
#include "9p/proto.h"
#include "9p/server.h"

#define SRV_MAX_FIDS 64
#define SRV_PATH_MAX 512
#define SRV_MSIZE 8192
#define SRV_PORT 9999

struct C9aux {
  uint8_t rx_buf[SRV_MSIZE];
  uint32_t rx_pos;
  uint8_t tx_buf[SRV_MSIZE];
  uint32_t tx_len;
  uint8_t client_ip[4];
  uint16_t client_port;
};

typedef struct {
  int in_use;
  C9fid fid;
  int vfs_fd;
  int is_dir;
  char path[SRV_PATH_MAX];
} srv_fid_t;

static C9ctx srv_ctx;
static struct C9aux srv_aux;
static srv_fid_t srv_fids[SRV_MAX_FIDS];
static vfs *srv_vfs;

static char str_nobody[] = "nobody";
static uint8_t empty_buf[1];

/* ---- transport callbacks ---- */

static uint8_t *srv_read(C9ctx *ctx, uint32_t size, int *err) {
  struct C9aux *a = ctx->aux;
  uint8_t *p = a->rx_buf + a->rx_pos;
  a->rx_pos += size;
  *err = 0;
  return p;
}

static uint8_t *srv_begin(C9ctx *ctx, uint32_t size) {
  struct C9aux *a = ctx->aux;
  a->tx_len = size;
  return a->tx_buf;
}

static int srv_end(C9ctx *ctx) {
  struct C9aux *a = ctx->aux;
  udp_send_packet(a->client_ip, SRV_PORT, a->client_port, a->tx_buf,
                  (int)a->tx_len);
  return 0;
}

static void srv_error(C9ctx *ctx, const char *fmt, ...) {
  (void)ctx;
  (void)fmt;
}

/* ---- fid table ---- */

static srv_fid_t *fid_get(C9fid fid) {
  int i;
  for (i = 0; i < SRV_MAX_FIDS; i++) {
    if (srv_fids[i].in_use && srv_fids[i].fid == fid)
      return &srv_fids[i];
  }
  return NULL;
}

static srv_fid_t *fid_alloc(C9fid fid) {
  int i;
  srv_fid_t *existing = fid_get(fid);
  if (existing) {
    if (existing->vfs_fd >= 0)
      srv_vfs->close(srv_vfs, existing->vfs_fd);
    existing->in_use = 0;
  }
  for (i = 0; i < SRV_MAX_FIDS; i++) {
    if (!srv_fids[i].in_use) {
      memset(&srv_fids[i], 0, sizeof(srv_fid_t));
      srv_fids[i].in_use = 1;
      srv_fids[i].fid = fid;
      srv_fids[i].vfs_fd = -1;
      return &srv_fids[i];
    }
  }
  return NULL;
}

static void fid_free(srv_fid_t *f) {
  f->in_use = 0;
  f->vfs_fd = -1;
}

/* ---- qid / stat helpers ---- */

static uint64_t path_hash(const char *path) {
  uint64_t h = 14695981039346656037ULL;
  for (; *path; path++) {
    h ^= (uint8_t)*path;
    h *= 1099511628211ULL;
  }
  return h;
}

static C9qid make_qid(const char *path, int is_dir) {
  C9qid q;
  memset(&q, 0, sizeof(q));
  q.path = path_hash(path);
  q.version = 0;
  q.type = is_dir ? C9qtdir : C9qtfile;
  return q;
}

static void path_join(char *out, const char *base, const char *name) {
  if (strcmp(base, "/") == 0) {
    out[0] = '/';
    strcpy(out + 1, name);
  } else {
    strcpy(out, base);
    strcat(out, "/");
    strcat(out, name);
  }
}

static char *path_last(const char *path) {
  const char *p = path;
  const char *last = path;
  while (*p) {
    if (*p == '/')
      last = p + 1;
    p++;
  }
  return (char *)last;
}

static C9stat make_c9stat(const char *name, const char *fullpath,
                           uint64_t size, int is_dir) {
  C9stat s;
  memset(&s, 0, sizeof(s));
  s.qid = make_qid(fullpath, is_dir);
  s.size = size;
  s.name = (char *)name;
  s.uid = str_nobody;
  s.gid = str_nobody;
  s.muid = str_nobody;
  s.mode = is_dir ? (uint32_t)(C9stdir | 0755u) : 0644u;
  return s;
}

/* ---- T-message handlers ---- */

static void handle_version(C9ctx *c, C9t *t) {
  (void)t;
  s9version(c);
}

static void handle_auth(C9ctx *c, C9t *t) {
  s9error(c, t->tag, "no auth");
}

static void handle_attach(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_alloc(t->fid);
  if (!f) {
    s9error(c, t->tag, "no free fids");
    return;
  }
  strcpy(f->path, "/");
  f->is_dir = 1;
  C9qid qid = make_qid("/", 1);
  s9attach(c, t->tag, &qid);
}

static void handle_walk(C9ctx *c, C9t *t) {
  srv_fid_t *src = fid_get(t->fid);
  if (!src) {
    s9error(c, t->tag, "unknown fid");
    return;
  }

  int nwalk = 0;
  while (t->walk.wname[nwalk] != NULL)
    nwalk++;

  if (nwalk > 0 && !src->is_dir) {
    s9error(c, t->tag, "not a directory");
    return;
  }

  char new_path[SRV_PATH_MAX];
  strcpy(new_path, src->path);

  C9qid qid_arr[C9maxpathel];
  C9qid *qid_ptrs[C9maxpathel + 1];
  int walked = 0;
  int walk_error = 0;

  int i;
  for (i = 0; i < nwalk; i++) {
    char candidate[SRV_PATH_MAX];
    path_join(candidate, new_path, t->walk.wname[i]);

    vfs_stat st;
    if (srv_vfs->stat(srv_vfs, candidate, &st) != VFS_SUCCESS) {
      if (walked == 0) {
        s9error(c, t->tag, "file not found");
        walk_error = 1;
      }
      break;
    }

    int is_dir = (st.type == FS_TYPE_DIRECTORY);
    qid_arr[walked] = make_qid(candidate, is_dir);
    qid_ptrs[walked] = &qid_arr[walked];
    strcpy(new_path, candidate);
    walked++;
  }
  qid_ptrs[walked] = NULL;

  if (walk_error)
    return;

  srv_fid_t *nf;
  if (t->walk.newfid == t->fid) {
    nf = src;
  } else {
    nf = fid_alloc(t->walk.newfid);
    if (!nf) {
      s9error(c, t->tag, "no free fids");
      return;
    }
  }

  strcpy(nf->path, new_path);
  nf->vfs_fd = -1;

  if (nwalk == 0) {
    nf->is_dir = src->is_dir;
  } else {
    vfs_stat st;
    nf->is_dir = 0;
    if (srv_vfs->stat(srv_vfs, new_path, &st) == VFS_SUCCESS)
      nf->is_dir = (st.type == FS_TYPE_DIRECTORY);
  }

  s9walk(c, t->tag, qid_ptrs);
}

static void handle_open(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_get(t->fid);
  if (!f) {
    s9error(c, t->tag, "unknown fid");
    return;
  }
  if (f->vfs_fd >= 0) {
    s9error(c, t->tag, "already open");
    return;
  }

  vfs_mode mode;
  int raw = t->open.mode & 3;
  if (raw == C9write)
    mode = VFS_WRITE;
  else if (raw == C9rdwr)
    mode = VFS_RDWR;
  else
    mode = VFS_READ;

  int fd;
  if (srv_vfs->open(srv_vfs, f->path, mode, &fd) != VFS_SUCCESS) {
    s9error(c, t->tag, "cannot open");
    return;
  }
  f->vfs_fd = fd;

  C9qid qid = make_qid(f->path, f->is_dir);
  uint32_t iounit = SRV_MSIZE - 11;
  s9open(c, t->tag, &qid, iounit);
}

static void handle_read(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_get(t->fid);
  if (!f) {
    s9error(c, t->tag, "unknown fid");
    return;
  }
  if (f->vfs_fd < 0) {
    s9error(c, t->tag, "not open");
    return;
  }

  if (f->is_dir) {
    if (t->read.offset > 0) {
      s9read(c, t->tag, empty_buf, 0);
      return;
    }

    static vfs_stat entries[32];
    int64_t count = srv_vfs->readdir(srv_vfs, f->vfs_fd, entries, 32);
    if (count < 0) {
      s9error(c, t->tag, "readdir failed");
      return;
    }

    static C9stat stats[32];
    static C9stat *stat_ptrs[33];
    static char name_bufs[32][256];
    int num = (int)count;
    int i;

    for (i = 0; i < num; i++) {
      char full_path[SRV_PATH_MAX];
      path_join(full_path, f->path, entries[i].name);
      int is_dir = (entries[i].type == FS_TYPE_DIRECTORY);
      strcpy(name_bufs[i], entries[i].name);
      stats[i] = make_c9stat(name_bufs[i], full_path, entries[i].size, is_dir);
      stat_ptrs[i] = &stats[i];
    }
    stat_ptrs[num] = NULL;

    uint64_t offset = 0;
    s9readdir(c, t->tag, stat_ptrs, &num, &offset, t->read.size);
  } else {
    static uint8_t read_buf[4096];
    int bytes_read = 0;

    srv_vfs->seek(srv_vfs, f->vfs_fd, (int64_t)t->read.offset, SEEK_SET);

    uint32_t to_read = t->read.size;
    if (to_read > sizeof(read_buf))
      to_read = (uint32_t)sizeof(read_buf);

    srv_vfs->read(srv_vfs, f->vfs_fd, read_buf, to_read, NULL, &bytes_read);
    s9read(c, t->tag, read_buf, (uint32_t)bytes_read);
  }
}

static void handle_write(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_get(t->fid);
  if (!f) {
    s9error(c, t->tag, "unknown fid");
    return;
  }
  if (f->vfs_fd < 0) {
    s9error(c, t->tag, "not open");
    return;
  }
  if (f->is_dir) {
    s9error(c, t->tag, "is a directory");
    return;
  }

  srv_vfs->seek(srv_vfs, f->vfs_fd, (int64_t)t->write.offset, SEEK_SET);
  int64_t written =
      srv_vfs->write(srv_vfs, f->vfs_fd, t->write.data, t->write.size);
  if (written < 0) {
    s9error(c, t->tag, "write failed");
    return;
  }
  s9write(c, t->tag, (uint32_t)written);
}

static void handle_stat(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_get(t->fid);
  if (!f) {
    s9error(c, t->tag, "unknown fid");
    return;
  }

  char *name = path_last(f->path);
  if (*name == '\0')
    name = "/";

  vfs_stat st;
  int ok = (srv_vfs->stat(srv_vfs, f->path, &st) == VFS_SUCCESS);
  if (!ok && f->vfs_fd >= 0)
    ok = (srv_vfs->fstat(srv_vfs, f->vfs_fd, &st) == VFS_SUCCESS);

  uint64_t sz = ok ? (uint64_t)st.size : 0;
  C9stat cs = make_c9stat(name, f->path, sz, f->is_dir);
  s9stat(c, t->tag, &cs);
}

static void handle_clunk(C9ctx *c, C9t *t) {
  srv_fid_t *f = fid_get(t->fid);
  if (!f) {
    s9error(c, t->tag, "unknown fid");
    return;
  }
  if (f->vfs_fd >= 0)
    srv_vfs->close(srv_vfs, f->vfs_fd);
  fid_free(f);
  s9clunk(c, t->tag);
}

static void handle_flush(C9ctx *c, C9t *t) { s9flush(c, t->tag); }

static void srv_handle_t(C9ctx *c, C9t *t) {
  switch (t->type) {
  case Tversion: handle_version(c, t); break;
  case Tauth:    handle_auth(c, t);    break;
  case Tattach:  handle_attach(c, t);  break;
  case Twalk:    handle_walk(c, t);    break;
  case Topen:    handle_open(c, t);    break;
  case Tread:    handle_read(c, t);    break;
  case Twrite:   handle_write(c, t);   break;
  case Tstat:    handle_stat(c, t);    break;
  case Tclunk:   handle_clunk(c, t);   break;
  case Tflush:   handle_flush(c, t);   break;
  default:       s9error(c, t->tag, "not supported"); break;
  }
}

/* ---- public API ---- */

void ninep_server_init(vfs *v) {
  srv_vfs = v;
  memset(&srv_ctx, 0, sizeof(srv_ctx));
  memset(&srv_aux, 0, sizeof(srv_aux));
  memset(srv_fids, 0, sizeof(srv_fids));
  srv_ctx.msize = SRV_MSIZE;
  srv_ctx.read = srv_read;
  srv_ctx.begin = srv_begin;
  srv_ctx.end = srv_end;
  srv_ctx.t = srv_handle_t;
  srv_ctx.error = srv_error;
  srv_ctx.aux = &srv_aux;
  KLOG(LOG_MODULE_9P, "9p: server ready on udp port %d", SRV_PORT);
}

void ninep_server_handle(void *data, uint32_t len, uint8_t *src_ip,
                          uint16_t src_port) {
  if (len > SRV_MSIZE)
    return;
  memcpy(srv_aux.rx_buf, data, len);
  memcpy(srv_aux.client_ip, src_ip, 4);
  srv_aux.client_port = src_port;
  srv_aux.rx_pos = 0;
  s9proc(&srv_ctx);
}
