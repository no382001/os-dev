#include "libc/types.h"
#include "9p.h"
#include "proto.h"
#include "libc/mem.h"

enum
{
	Svver = 1<<0,
};

static uint8_t *
R(C9ctx *c, uint32_t size, C9rtype type, C9tag tag, C9error *err)
{
	uint8_t *p = NULL;

	if(size > c->msize-4-1-2){
		c->error(c, "R: invalid size %ud", size);
		*err = C9Esize;
	}else{
		size += 4+1+2;
		if((p = c->begin(c, size)) == NULL){
			c->error(c, "R: no buffer for %ud bytes", size);
			*err = C9Ebuf;
		}else{
			*err = 0;
			w32(&p, size);
			w08(&p, type);
			w16(&p, tag);
		}
	}
	return p;
}

C9error
s9proc(C9ctx *c)
{
	uint32_t i, sz, cnt, n, msize;
	int readerr;
	uint8_t *b;
	C9error err;
	C9t t;

	readerr = -1;
	if((b = c->read(c, 4, &readerr)) == NULL){
		if(readerr != 0)
			c->error(c, "s9proc: short read");
		return readerr == 0 ? 0 : C9Epkt;
	}

	sz = r32(&b);
	if(sz < 7 || sz > c->msize){
		c->error(c, "s9proc: invalid packet size !(7 <= %ud <= %ud)", sz, c->msize);
		return C9Epkt;
	}
	sz -= 4;
	readerr = -1;
	if((b = c->read(c, sz, &readerr)) == NULL){
		if(readerr != 0)
			c->error(c, "s9proc: short read");
		return readerr == 0 ? 0 : C9Epkt;
	}

	t.type = r08(&b);
	t.tag = r16(&b);
	sz -= 3;

	if((c->svflags & Svver) == 0 && t.type != Tversion){
		c->error(c, "s9proc: expected Tversion, got %ud", t.type);
		return C9Epkt;
	}

	switch(t.type){
	case Tread:
		if(sz < 4+8+4)
			goto error;
		t.fid = r32(&b);
		t.read.offset = r64(&b);
		t.read.size = r32(&b);
		if(t.read.size > maxread(c))
		  t.read.size = maxread(c);
		c->t(c, &t);
		break;

	case Twrite:
		if(sz < 4+8+4)
			goto error;
		t.fid = r32(&b);
		t.write.offset = r64(&b);
		if((t.write.size = r32(&b)) < sz-4-8-4)
			goto error;
		if(t.write.size > maxwrite(c))
		  t.write.size = maxwrite(c);
		t.write.data = b;
		c->t(c, &t);
		break;

	case Tclunk:
	case Tstat:
	case Tremove:
		if(sz < 4)
			goto error;
		t.fid = r32(&b);
		c->t(c, &t);
		break;

	case Twalk:
		if(sz < 4+4+2)
			goto error;
		t.fid = r32(&b);
		t.walk.newfid = r32(&b);
		if((n = r16(&b)) > 16){
			c->error(c, "s9proc: Twalk !(%ud <= 16)", n);
			return C9Epath;
		}
		sz -= 4+4+2;
		if(n > 0){
			for(i = 0; i < n; i++){
				if(sz < 2 || (cnt = r16(&b)) > sz-2)
					goto error;
				if(cnt < 1){
					c->error(c, "s9proc: Twalk invalid element [%ud]", i);
					return C9Epath;
				}
				b[-2] = 0;
				t.walk.wname[i] = (char*)b;
				b += cnt;
				sz -= 2 + cnt;
			}
			memmove(t.walk.wname[i-1]-1, t.walk.wname[i-1], (char*)b - t.walk.wname[i-1]);
			t.walk.wname[i-1]--;
			b[-1] = 0;
		}else
			i = 0;
		t.walk.wname[i] = NULL;
		c->t(c, &t);
		break;

	case Topen:
		if(sz < 4+1)
			goto error;
		t.fid = r32(&b);
		t.open.mode = r08(&b);
		c->t(c, &t);
		break;

	case Twstat:
		if(sz < 4+2)
			goto error;
		t.fid = r32(&b);
		if((cnt = r16(&b)) > sz-4)
			goto error;
		if((err = c9parsedir(c, &t.wstat, &b, &cnt)) != 0){
			c->error(c, "s9proc");
			return err;
		}
		c->t(c, &t);
		break;

	case Tcreate:
		if(sz < 4+2+4+1)
			goto error;
		t.fid = r32(&b);
		if((cnt = r16(&b)) < 1 || cnt > sz-4-2-4-1)
			goto error;
		t.create.name = (char*)b;
		b += cnt;
		t.create.perm = r32(&b);
		t.create.mode = r08(&b);
		t.create.name[cnt] = 0;
		c->t(c, &t);
		break;

	case Tflush:
		if(sz < 2)
			goto error;
		t.flush.oldtag = r16(&b);
		c->t(c, &t);
		break;

	case Tversion:
		if(sz < 4+2 || (msize = r32(&b)) < C9minmsize || (cnt = r16(&b)) > sz-4-2)
			goto error;
		if(cnt < 6 || memcmp(b, (uint8_t*)"9P2000", 6) != 0){
			if((b = R(c, 4+2+7, Rversion, 0xffff, &err)) != NULL){
				w32(&b, 0);
				wcs(&b, "unknown", 7);
				err = c->end(c);
				c->error(c, "s9proc: invalid version");
			}
			return C9Ever;
		}
		if(msize < c->msize)
			c->msize = msize;
		c->svflags |= Svver;
		c->t(c, &t);
		break;

	case Tattach:
		if(sz < 4+4+2+2)
			goto error;
		t.fid = r32(&b);
		t.attach.afid = r32(&b);
		cnt = r16(&b);
		sz -= 4+4+2;
		if(cnt+2 > sz)
			goto error;
		t.attach.uname = (char*)b;
		b += cnt;
		cnt = r16(&b);
		b[-2] = 0;
		sz -= cnt+2;
		if(cnt > sz)
			goto error;
		memmove(b-1, b, cnt);
		t.attach.aname = (char*)b-1;
		t.attach.aname[cnt] = 0;
		c->t(c, &t);
		break;

	case Tauth:
		if(sz < 4+2+2)
			goto error;
		t.auth.afid = r32(&b);
		cnt = r16(&b);
		sz -= 4+2;
		if(cnt+2 > sz)
			goto error;
		t.auth.uname = (char*)b;
		b += cnt;
		cnt = r16(&b);
		b[-2] = 0;
		sz -= cnt+2;
		if(cnt > sz)
			goto error;
		memmove(b-1, b, cnt);
		t.auth.aname = (char*)b-1;
		t.auth.aname[cnt] = 0;
		c->t(c, &t);
		break;

	default:
		goto error;
	}
	return 0;
error:
	c->error(c, "s9proc: invalid packet (type=%ud)", t.type);
	return C9Epkt;
}


C9error
c9parsedir(C9ctx *c, C9stat *stat, uint8_t **t, uint32_t *size)
{
	uint8_t *b;
	uint32_t cnt, sz;

	sz = 0;
	if(*size < 49 || (sz = r16(t)) < 47 || *size < 2+sz)
		goto error;
	*size -= 2+sz;
	*t += 6; /* skip type(2) and dev(4) */
	stat->qid.type = r08(t);
	stat->qid.version = r32(t);
	stat->qid.path = r64(t);
	stat->mode = r32(t);
	stat->atime = r32(t);
	stat->mtime = r32(t);
	stat->size = r64(t);
	sz -= 39;
	if((cnt = r16(t)) > sz-2)
		goto error;
	stat->name = (char*)*t; b = *t = *t+cnt; sz -= 2+cnt;
	if(sz < 2 || (cnt = r16(t)) > sz-2)
		goto error;
	stat->uid = (char*)*t; *b = 0; b = *t = *t+cnt; sz -= 2+cnt;
	if(sz < 2 || (cnt = r16(t)) > sz-2)
		goto error;
	stat->gid = (char*)*t; *b = 0; b = *t = *t+cnt; sz -= 2+cnt;
	if(sz < 2 || (cnt = r16(t)) > sz-2)
		goto error;
	stat->muid = memmove(*t-1, *t, cnt); *b = stat->muid[cnt] = 0; *t = *t+cnt; sz -= 2+cnt;
	*t += sz;
	return 0;
error:
	c->error(c, "c9parsedir: invalid size: size=%ud sz=%ud", *size, sz);
	return C9Epkt;
}