#include "9p_prot.h"
#include "libc/mem.h"
#include "libc/string.h"

enum {
  Svver = 1 << 0,
};

#define safestrlen(s) (s == NULL ? 0 : (uint32_t)strlen(s))
#define maxread(c) (c->msize - 4 - 4 - 1 - 2)
#define maxwrite(c) maxread(c)

static void w08(uint8_t **p, uint8_t x) {
  (*p)[0] = x;
  *p += 1;
}

static void w16(uint8_t **p, uint16_t x) {
  (*p)[0] = x;
  (*p)[1] = x >> 8;
  *p += 2;
}

static void w32(uint8_t **p, uint32_t x) {
  (*p)[0] = x;
  (*p)[1] = x >> 8;
  (*p)[2] = x >> 16;
  (*p)[3] = x >> 24;
  *p += 4;
}

static void w64(uint8_t **p, uint64_t x) {
  (*p)[0] = x;
  (*p)[1] = x >> 8;
  (*p)[2] = x >> 16;
  (*p)[3] = x >> 24;
  (*p)[4] = x >> 32;
  (*p)[5] = x >> 40;
  (*p)[6] = x >> 48;
  (*p)[7] = x >> 56;
  *p += 8;
}

static void wcs(uint8_t **p, const char *s, int len) {
  w16(p, len);
  if (s != NULL) {
    memmove(*p, s, len);
    *p += len;
  }
}

static uint8_t r08(uint8_t **p) {
  *p += 1;
  return (*p)[-1];
}

static uint16_t r16(uint8_t **p) {
  *p += 2;
  return (*p)[-2] << 0 | (*p)[-1] << 8;
}

static uint32_t r32(uint8_t **p) {
  *p += 4;
  return (*p)[-4] << 0 | (*p)[-3] << 8 | (*p)[-2] << 16 |
         (uint32_t)((*p)[-1]) << 24;
}

static uint64_t r64(uint8_t **p) {
  uint64_t v;

  v = r32(p);
  v |= (uint64_t)r32(p) << 32;
  return v;
}

static uint8_t *R(nine_ctx *c, uint32_t size, nine_r_type_t type,
                  nine_tag_t tag, nine_error *err) {
  uint8_t *p = NULL;

  if (size > c->msize - 4 - 1 - 2) {
    c->error(c, "R: invalid size %ud", size);
    *err = C9Esize;
  } else {
    size += 4 + 1 + 2;
    if ((p = c->begin(c, size)) == NULL) {
      c->error(c, "R: no buffer for %ud bytes", size);
      *err = C9Ebuf;
    } else {
      *err = 0;
      w32(&p, size);
      w08(&p, type);
      w16(&p, tag);
    }
  }
  return p;
}

nine_error s9version(nine_ctx *c) {
  uint8_t *b;
  nine_error err;

  if ((b = R(c, 4 + 2 + 6, Rversion, 0xffff, &err)) != NULL) {
    w32(&b, c->msize);
    wcs(&b, "9P2000", 6);
    err = c->end(c);
  };
  return err;
}

nine_error s9error(nine_ctx *c, nine_tag_t tag, const char *ename) {
  uint32_t len = safestrlen(ename);
  uint8_t *b;
  nine_error err;

  if (len > C9maxstr) {
    c->error(c, "s9error: invalid ename: %ud chars", len);
    return C9Estr;
  }
  if ((b = R(c, 2 + len, Rerror, tag, &err)) != NULL) {
    wcs(&b, ename, len);
    err = c->end(c);
  }
  return err;
}