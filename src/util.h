/* 
 * See LICENSE file for copyright and license details. 
 */
#include <stdio.h>

enum type {
	BOOL,
	STRING,
};

/* NOTE: Read only %s from format. */
char       *make_post_request(const char *, const char *, const size_t,
                              const char *, ...);
ssize_t    safe_tls_read(struct tls *, void *, size_t);
ssize_t    safe_tls_write(struct tls *, const void *, size_t);
int        unparse_json_field(const char *, const char *, enum type, void *);
