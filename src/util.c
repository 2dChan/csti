/*
 * See LICENSE file for copyright and license details.
 */
#include <tls.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char 
*make_post_request(const char *host, const char *content_type,
                   const size_t additonal_lenght, const char *data_format, ...)
{
	static const char post_request_template[] = 
		"POST /cgi-bin/new-client HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Conncetion: keep-alive\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %lu\r\n"
		"\r\n";
	
	size_t content_lenght, lenght;
	char *request, *arg, *write_ptr;
	const char *ptr;

	va_list args;
	va_start(args, data_format);

	content_lenght = strlen(data_format) + additonal_lenght;
	ptr = data_format;
	while (1) {
		while (*ptr && *ptr++ != '%');
		if (*ptr == 0)
			break;

		switch (*ptr) {
		case 's':
			arg = va_arg(args, char *);
			// Sub 2 because in data_format include %s.
			content_lenght += strlen(arg) - 2;
		case '%':
			continue;
		default:
			fprintf(stderr, "make_post_request: Unsuported format %%%c", *ptr);
		}
	}

	lenght = sizeof(post_request_template) + strlen(host) +
		strlen(content_type) + sizeof(content_lenght) + content_lenght;

	request = malloc(lenght);
	if (request == NULL) {
		perror("malloc");
		return NULL;
	}

	lenght = sprintf(request, post_request_template, host, content_type,
	                 content_lenght);

	ptr = data_format;
	write_ptr = request + lenght;
	va_start(args, data_format);
	while (*ptr) {
		if (strncmp(ptr, "%s", 2) == 0) {
			arg = va_arg(args, char *);
			lenght = strlen(arg);
			memcpy(write_ptr, arg, lenght);
			write_ptr += lenght;
			ptr += 2;
			continue;
		}

		*write_ptr++ = *ptr++;
		*write_ptr = 0;
	}
	*write_ptr = 0;

	return request;
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


ssize_t
tls_safe_read(struct tls *ctx, void *buf, size_t len)
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
tls_safe_write(struct tls *ctx, const void *buf, size_t len)
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
