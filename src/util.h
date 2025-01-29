/* 
 * See LICENSE file for copyright and license details. 
 */
#include <stdio.h>

enum type {
	BOOL,
	STRING,
};

/* NOTE: Read only %s from format. */
ssize_t    tls_safe_read(struct tls *, void *, size_t);
ssize_t    tls_safe_write(struct tls *, const void *, size_t);

char       *make_post_request(const char *, const char *, const size_t,
                              const char *, ...);
int        unparse_json_field(const char *, const char *, enum type, void *);
