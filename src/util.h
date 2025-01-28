#include <stdio.h>

enum type {
	BOOL,
	STRING,
};

ssize_t    safe_tls_read(struct tls *, void *, size_t);
ssize_t    safe_tls_write(struct tls *, const void *, size_t);

int        unparse_json_field(const char *, const char *, enum type, void *);
