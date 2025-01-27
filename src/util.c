#include <stdio.h>
#include <string.h>
#include <tls.h>

#include "util.h"

ssize_t
safe_tls_read(struct tls *ctx, void *buf, size_t len)
{
	/* 
	 * TODO:
	 * 1) Partional read.
	 * 2) Refactor.
	 */
	ssize_t read_len = 0, curr_len;

    while (len > read_len) {
		curr_len = tls_read(ctx, buf, len);

		switch (curr_len) {
		case TLS_WANT_POLLIN:
		case TLS_WANT_POLLOUT:
			continue;
		case -1:
			printf("tls_read %s\n", tls_error(ctx));
			return -1;
		case 0:
			break;
		}
		buf += curr_len;
		read_len += curr_len;	

		break;
	}

	return read_len;
}

ssize_t
safe_tls_write(struct tls *ctx, const void *buf, size_t len)
{
	ssize_t write_len = 0, curr_len;

    while (len > write_len) {
		curr_len = tls_write(ctx, buf, len);

		switch (curr_len) {
		case TLS_WANT_POLLIN:
		case TLS_WANT_POLLOUT:
			continue;
		case -1:
			fprintf(stderr, "tls_write %s", tls_error(ctx));
			return -1;
		}
		buf += curr_len;
		write_len += curr_len;
	}

	return write_len;
}


void 
unparse_json_field(const char *json, const char *name, char *value, size_t lenght) 
{
	/*
	 * TODO: 
	 * 1) Ignore whitespace.
	 * 2) Error handle.
	 * 3) Unparse all types int/bool/string(now only string fields).
	 * 4) Refactor.
	 */
	const char *ptr;

	ptr = strstr(json, name);
	ptr = strchr(ptr, ':');
	ptr = strchr(ptr, '"') + 1;
	strncpy(value, ptr, lenght);
	value[lenght] = 0;
}

