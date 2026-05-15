#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int
dial(const char *s)
{
	struct addrinfo *r, *a, hint = {.ai_flags = AI_ADDRCONFIG, .ai_family = AF_UNSPEC, 0};
	char host[128], *port;
	int e, f;

	if(strncmp(s, "udp!", 4) == 0){
		hint.ai_socktype = SOCK_DGRAM;
		hint.ai_protocol = IPPROTO_UDP;
	}else if(strncmp(s, "tcp!", 4) == 0){
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
	}else{
		fprintf(stderr, "invalid dial string: %s\n", s);
		return -1;
	}
	if((port = strchr(s+4, '!')) == NULL){
		fprintf(stderr, "invalid dial string: %s\n", s);
		return -1;
	}
	if(snprintf(host, sizeof(host), "%.*s", (int)(port-s-4), s+4) >= (int)sizeof(host)){
		fprintf(stderr, "host name too large: %s\n", s);
		return -1;
	}
	port++;
	if((e = getaddrinfo(host, port, &hint, &r)) != 0){
		fprintf(stderr, "%s: %s\n", gai_strerror(e), s);
		return -1;
	}
	f = -1;
	for(a = r; a != NULL; a = a->ai_next){
		if((f = socket(a->ai_family, a->ai_socktype, a->ai_protocol)) < 0)
			continue;
		if(connect(f, a->ai_addr, a->ai_addrlen) == 0)
			break;
		close(f);
		f = -1;
	}
	freeaddrinfo(r);

	return f;
}
