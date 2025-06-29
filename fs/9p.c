#include "9p.h"
#include "9p_prot.h"
#include "drivers/serial.h"

static uint8_t *ctxread(nine_ctx *ctx, uint32_t size, int *err) { return 0; }
static uint8_t *ctxbegin(nine_ctx *ctx, uint32_t size) { return 0; }

static int ctxend(nine_ctx *ctx) { return 0; }

static void ctxerror(nine_ctx *ctx, const char *fmt, ...) {}

static void init_nine_aux(nine_aux *c) {

  // c->f = f;
  c->c.read = ctxread;
  c->c.begin = ctxbegin;
  c->c.end = ctxend;
  c->c.error = ctxerror;
  c->c.aux = c;
}

void handlereq(nine_ctx *c, nine_transmit *t) {

  switch (t->type) {
  case Tversion:
    serial_debug("version request");
    s9version(c);
    break;

  case Tattach:
    serial_debug("attach request: fid=%d, uname=%s, aname=%s", t->fid,
                 t->attach.uname, t->attach.aname);

    // s9attach(c, t->tag, &qid);
    break;

  case Twalk:
    break;

  case Topen:
    break;

  case Tread:
    break;

  case Tstat:
    break;

  case Tclunk:
    break;

  default:
    serial_debug("unhandled request type: %d\n", t->type);
    s9error(c, t->tag, "operation not supported");
    break;
  }
}