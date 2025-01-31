/*
 * See LICENSE file for copyright and license details.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tls.h>

#include "util.h"

#define BUFFER_SIZE 1024

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

ssize_t
make_post_request(char *request, const char *host, const char *content_type,
	const size_t additonal_lenght, const char *data_format, ...)
{
	static const char template[] = "POST /cgi-bin/new-client HTTP/1.1\r\n"
								   "Host: %s\r\n"
								   "Conncetion: keep-alive\r\n"
								   "Content-Type: %s\r\n"
								   "Content-Length: %lu\r\n"
								   "\r\n"
								   "%s";

	char content[BUFFER_SIZE];
	ssize_t lenght;

	va_list args;
	va_start(args, data_format);
	lenght = vsprintf(content, data_format, args);
	if (lenght > BUFFER_SIZE || lenght == -1) {
		perror("vsprintf");
		return -1;
	}

	lenght = sprintf(request, template, host, content_type,
		lenght + additonal_lenght, content);
	if (lenght < 0) {
		perror("spinrtf");
		return -1;
	}

	return lenght;
}

void
unpack_header(char *header, char **contest_id, char **prob_id)
{
	char *ptr;

	*contest_id = NULL, *prob_id = NULL;
	for (ptr = header; *ptr != 0; ++ptr) {
		if (isspace(*ptr) || *ptr == '-') {
			*ptr = 0;
			if (*contest_id == NULL) {
				*contest_id = ptr + 1;
			} else if (*prob_id == NULL) {
				*prob_id = ptr + 1;
			} else {
				break;
			}
		}
	}
}

int
unparse_json_field(const char *json, const char *name, enum type type,
	void *value)
{
	/* NOTE: Not unparse string with '"'. */
	size_t distanse = 0;
	char *ptr, *ptr1;

	ptr = strstr(json, name);
	if (ptr == NULL)
		return 2;
	ptr = strchr(ptr, ':');
	if (ptr == NULL)
		goto failure;

	switch (type) {
	case BOOL:
		while (isalpha(*ptr) == 0 && *ptr++)
			;
		if (*ptr == 0)
			goto failure;

		*(int *)value = (*ptr == 't');
		break;
	case STRING:
		ptr = strchr(ptr, '"');
		if (ptr == NULL || *++ptr == 0)
			goto failure;

		ptr1 = strchr(ptr, '"');
		if (ptr1 == NULL)
			goto failure;

		distanse = ptr1 - ptr;
		snprintf(value, distanse + 1, "%s", ptr);
		break;
	case INT:
		while (isdigit(*ptr) == 0 && *ptr++)
			;
		if (*ptr == 0)
			goto failure;
		sscanf(ptr, "%d", (int *)value);
		break;
	case UINT:
		while (isdigit(*ptr) == 0 && *ptr++)
			;
		if (*ptr == 0)
			goto failure;
		sscanf(ptr, "%u", (unsigned int *)value);
		break;
	default:
		fprintf(stderr, "unparse_json_field: Unclassifed type\n");
	}

	return 0;

failure:
	fprintf(stderr, "unparse_json_field: Json parsing error\n");
	return 1;
}

const char *
get_problem_status_name(const enum problem_status status)
{
	switch (status) {
	case OK:
		return "Ok";
	case COMPILATION_ERROR:
		return "Compilation error";
	case RUN_TIME_ERROR:
		return "Run-time error";
	case TIME_LIMIT_EXCEEDED:
		return "Time-limit exceeded";
	case PRESENTATION_ERROR:
		return "Presentation error";
	case WRONG_ANSWER:
		return "Wrong answer";
	case CHECK_FAILED:
		return "Check failed";
	case PARITIAL_SOLUTION:
		return "Partial solution";
	case ACCEPTED_FOR_TESTING:
		return "Accepted for testing";
	case IGNORED:
		return "Ignored";
	case DISQUALIFIED:
		return "Disqualified";
	case PENDING_CHECK:
		return "Pending check";
	case MEMORY_LIMIT_EXCEEDED:
		return "Memory limit exceeded";
	case SECURITY_VIOLATION:
		return "Security violation";
	case CODING_STYLE_VIOLATION:
		return "Coding style violation";
	case WALL_TIME_LIMIT_EXCEEDED:
		return "Wall time-limit exceeded";
	case PENDING_REVIEW:
		return "Pending review";
	case REJECTED:
		return "Rejected";
	default:
		return "Unknown";
	}
}
