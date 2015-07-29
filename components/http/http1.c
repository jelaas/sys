#include <unistd.h>
#include <poll.h>
#include <ctype.h>

#include "duktape.h"
#include "http1.h"

struct hdr {
	char *name;
	char *value;
};

#define MAX_HDR 32
static struct {
	int hdr_fd;
	struct hdr headers[MAX_HDR];
	int status;
	const char *message;
} conf;

static int put_header(struct hdr *hdr)
{
	int i, empty = -1;

	for(i=0;i<MAX_HDR;i++) {
		if(conf.headers[i].name) {
			if(!strcasecmp(conf.headers[i].name, hdr->name)) {
				conf.headers[i].value = hdr->value;
				return i;
			}
		} else {
			if(empty == -1)
				empty = i;
		}
	}
	if(empty >= 0) {
		memcpy(&conf.headers[empty], hdr, sizeof(struct hdr));
		return empty;
	}
	return -1;
}

static int write_headers()
{
	int i;
	char buf[256];
	
	snprintf(buf, sizeof(buf), "Status: %d %s\r\n", conf.status, conf.message);
	write(1, buf, strlen(buf));
	for(i=0;i<MAX_HDR;i++) {
		if(conf.headers[i].name) {
			write(1, conf.headers[i].name, strlen(conf.headers[i].name));
			write(1, ": ", 2);
			write(1, conf.headers[i].value, strlen(conf.headers[i].value));
			write(1, "\r\n", 2);
		}
	}
	write(1, "\r\n", 2);
	return 0;
}

static char *wash(const char *s)
{
	char *h, *p;
	h = strdup(s);
	if(!h) return (void *)0;
	for(p=h;*p;p++) {
		if(*p == '\n') {
			if(*(p+1))
				*p = ' ';
			else
				*p = 0;
		}
	}
	return h;
}

static int http1_header(duk_context *ctx)
{
	const char *hdr = duk_to_string(ctx, 0);
	char *h;
	int rc;

	h = wash(hdr);

	if(h) {
		rc = write(conf.hdr_fd, h, strlen(h));
		if(rc > 0) {
			rc = write(conf.hdr_fd, "\n", 1);
		}
	} else {
		rc = -1;
	}
	
	duk_push_int(ctx, rc > 0);
	free(h);
	return 1;
}

static int http1_status(duk_context *ctx)
{
	char buf[512], *p;
	int rc, code = duk_to_int(ctx, 0);
	const char *message = duk_to_string(ctx, 1);
	
	snprintf(buf, sizeof(buf), "%d %s\n", code, message);
	if( (p=strchr(buf, '\n')) )
		*(p+1)=0;
	rc = write(conf.hdr_fd, buf, strlen(buf));
	
	duk_push_int(ctx, rc > 0);	
	return 1;
}


int hdr_parse(struct hdr *hdr, const char *s)
{
	char *p;
	p = strchr(s, ':');
	if(!p) return -1;
	hdr->name = strndup(s, p-s);
	for(p=p+1;*p;p++) {
		if(*p == ' ') continue;
		if(*p == '\t') continue;
		break;
	}
	hdr->value = strndup(p, strlen(p));
	return 0;
}

static int http1_init(duk_context *ctx)
{
	// open pipes for header and content
	int fds_hdr[2], fds_cont[2];
	pid_t pid;
	
	conf.status = 200;
	conf.message = "Ok";
	
	pipe(fds_hdr);
	conf.hdr_fd = fds_hdr[1];
	pipe(fds_cont);
	
	pid = fork();
	if(pid) {
		dup2(fds_cont[1], 1);
		close(fds_cont[0]);
		close(fds_hdr[0]);
	} else {
		/*
		 * separate process to handle headers and content
		 */
		char *content;
		size_t content_bufsize;
		size_t content_size = 0;
		char *header;
		size_t header_bufsize;
		size_t header_size = 0;
		ssize_t n;
		int rc;
		struct pollfd pollfds[2];
		int nfds = 2;
		struct hdr hdr;
		
		close(fds_cont[1]);
		close(fds_hdr[1]);
		
		content_bufsize = 4096;
		content = malloc(content_bufsize);
		header_bufsize = 4096;
		header = malloc(header_bufsize);
		
		pollfds[0].fd = fds_cont[0];
		pollfds[1].fd = fds_hdr[0];
		pollfds[0].events = POLLIN;
		pollfds[1].events = POLLIN;
		
		while( (rc = poll(pollfds, nfds, -1)) > 0) {
			if(pollfds[0].revents) {
				if( (content_bufsize - content_size) < 1024 ) {
					content_bufsize *= 2;
					content = realloc(content, content_bufsize);
				}
				n = read(fds_cont[0], content + content_size,
					 content_bufsize - content_size);
				if(n <= 0) break;
				content_size += n;
			}
			if(pollfds[1].revents) {
				char *p;
				if( (header_bufsize - header_size) < 1025 ) {
					header_bufsize *= 2;
					header = realloc(header, header_bufsize);
				}
				n = read(fds_hdr[0], header + header_size,
					 header_bufsize - header_size - 1);
				if(n > 0) {
					header_size += n;
					header[header_size] = 0;
					// FIXME! loop over newlines
					while( (p=strchr(header, '\n')) ) {
						int len;
						len = p-header+1;
						*p = 0;
						if(isdigit(*header)) {
							if( (p=strchr(header, ' '))) {
									conf.status = atoi(header);
									conf.message = strdup(p+1);
								}
						} else {
							hdr_parse(&hdr, header);
							put_header(&hdr);
						}
						memmove(header, header+len, header_size-len);
						header_size -= len;
					}
				} else {
					write_headers();
					close(fds_hdr[0]);
					nfds = 1;
				}					
			}
		}
		if(nfds == 2) {
			char buf[32];
			hdr.name = "Content-Length";
			sprintf(buf, "%zu", content_size);
			hdr.value = buf;
			put_header(&hdr);
			write_headers();
		}
		write(1, content, content_size);
		_exit(0);
	}	
	duk_push_int(ctx, 0);
	return 1;
}

static const duk_function_list_entry http1_funcs[] = {
	{ "header", http1_header, 1 /* header */ },
	{ "init", http1_init, 0 /* header */ },
	{ "status", http1_status, 2 /* code, message */ },
	{ NULL, NULL, 0 }
};

static const duk_number_list_entry http1_consts[] = {
	{ NULL, 0.0 }
};

int http1_load(duk_context *ctx, int n, struct prg *prg)
{
	duk_push_object(ctx);  /* -> [ ... http obj ] */
	duk_put_function_list(ctx, -1, http1_funcs);
	duk_put_number_list(ctx, -1, http1_consts);
	duk_put_prop_string(ctx, n, "CGI");  /* -> [ ... http ] */
	return 0;
}
