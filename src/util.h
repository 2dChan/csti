/*
 * See LICENSE file for copyright and license details.
 */
#include <stdio.h>

#define BOUNDARY "--CstiBoundaryR23cA9G3dD229"

#define BODY_PART_TEMPLATE(name, value) \
	"--" BOUNDARY "\r\n"                \
	"Content-Disposition: form-data; name=\"" name "\"\r\n\r\n" value

#define GET_REQUEST_TEMPLATE(args)                                         \
	"GET /cgi-bin/new-client?json=1&SID=%s&EJSID=%s&" args " HTTP/1.1\r\n" \
	"Host: %s\r\n"                                                         \
	"Connection: keep-alive\r\n"                                           \
	"\r\n";

enum type {
	BOOL,
	STRING,
	INT,
	UINT,
};

ssize_t tls_safe_read(struct tls *, void *, size_t);
ssize_t tls_safe_write(struct tls *, const void *, size_t);

ssize_t make_post_request(char *, const char *, const char *, const char *,
	const size_t, const char *, ...);

int unparse_json_field(const char *, const char *, enum type, void *);
