#include "libc/types.h"
#include "libc/mem.h"

void
w08(uint8_t **p, uint8_t x)
{
	(*p)[0] = x;
	*p += 1;
}

void
w16(uint8_t **p, uint16_t x)
{
	(*p)[0] = x;
	(*p)[1] = x>>8;
	*p += 2;
}

void
w32(uint8_t **p, uint32_t x)
{
	(*p)[0] = x;
	(*p)[1] = x>>8;
	(*p)[2] = x>>16;
	(*p)[3] = x>>24;
	*p += 4;
}

void
w64(uint8_t **p, uint64_t x)
{
	(*p)[0] = x;
	(*p)[1] = x>>8;
	(*p)[2] = x>>16;
	(*p)[3] = x>>24;
	(*p)[4] = x>>32;
	(*p)[5] = x>>40;
	(*p)[6] = x>>48;
	(*p)[7] = x>>56;
	*p += 8;
}

void
wcs(uint8_t **p, const char *s, int len)
{
	w16(p, len);
	if(s != NULL){
		memmove(*p, s, len);
		*p += len;
	}
}

uint8_t
r08(uint8_t **p)
{
	*p += 1;
	return (*p)[-1];
}

uint16_t
r16(uint8_t **p)
{
	*p += 2;
	return (*p)[-2]<<0 | (*p)[-1]<<8;
}

uint32_t
r32(uint8_t **p)
{
	*p += 4;
	return (*p)[-4]<<0 | (*p)[-3]<<8 | (*p)[-2]<<16 | (uint32_t)((*p)[-1])<<24;
}

uint64_t
r64(uint8_t **p)
{
	uint64_t v;

	v = r32(p);
	v |= (uint64_t)r32(p)<<32;
	return v;
}