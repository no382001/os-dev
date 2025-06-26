#define NINEP2000_PORT 9999

#define safestrlen(s) (s == NULL ? 0 : (uint32_t)strlen(s))
#define maxread(c) (c->msize-4-4-1-2)
#define maxwrite(c) maxread(c)

void ninep_handle_packet(void* data, uint16_t length);