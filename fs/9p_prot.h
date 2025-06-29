#pragma once
#include "9p.h"

nine_error s9version(nine_ctx *c);
nine_error s9error(nine_ctx *c, nine_tag_t tag, const char *ename);