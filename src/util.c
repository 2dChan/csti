/*
 * See LICENSE file for copyright and license details.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>

#include "util.h"

ssize_t
safe_tls_read(struct tls *ctx, void *buf, size_t len)
{
	ssize_t read_len;

    do {
		read_len = tls_read(ctx, buf, len);
		if (read_len == -1) {
			printf("tls_read: %s\n", tls_error(ctx));
			return -1;
		}
	} while (read_len == TLS_WANT_POLLIN || read_len == TLS_WANT_POLLOUT);

	return read_len;
}

ssize_t
safe_tls_write(struct tls *ctx, const void *buf, size_t len)
{
	ssize_t write_len;

    do {
		write_len = tls_write(ctx, buf, len);
		if (write_len == -1) {
			printf("tls_read: %s\n", tls_error(ctx));
			return -1;
		}
	} while (write_len == TLS_WANT_POLLIN || write_len == TLS_WANT_POLLOUT);

	return write_len;
}

int
unparse_json_field(const char *json, const char *name, enum type type, void *value)
{
	/* NOTE: Not unparse string with '"'. */
	size_t distanse = 0;
	char *ptr, *ptr1;

	ptr = strstr(json, name);
	if (ptr == NULL)
		goto failure;
	ptr = strchr(ptr, ':');
	if (ptr == NULL)
		goto failure;

	switch (type) {
	case BOOL:
		while(isalpha(*ptr) == 0 && *ptr++);
		if (*ptr == 0)
			goto failure;

		*(int*)value = (*ptr == 't');
		break;
	case STRING:
		ptr = strchr(ptr, '"');
		if(ptr == NULL || *++ptr == 0)
			goto failure;

		ptr1 = strchr(ptr, '"');
		if (ptr1 == NULL)
			goto failure;

		distanse = ptr1 - ptr;
		snprintf(value, distanse + 1, "%s", ptr);
		break;
	default:
		fprintf(stderr, "unparse_json_field: Unclassifed type\n");	
	}

	return 0;

failure:
	fprintf(stderr, "unparse_json_field: Json parsing error\n");
	return 1;
}

