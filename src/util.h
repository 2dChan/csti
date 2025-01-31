/*
 * See LICENSE file for copyright and license details.
 */
#include <stdio.h>

#define GET_REQUEST_TEMPLATE(args)                                             \
	"GET /cgi-bin/new-client?json=1&SID=%s&EJSID=%s&" args " HTTP/1.1\r\n" \
	"Host: %s\r\n"                                                         \
	"Connection: keep-alive\r\n"                                           \
	"\r\n";

enum problem_status {
	OK = 0,
	COMPILATION_ERROR = 1,
	RUN_TIME_ERROR = 2,
	TIME_LIMIT_EXCEEDED = 3,
	PRESENTATION_ERROR = 4,
	WRONG_ANSWER = 5,
	CHECK_FAILED = 6,
	PARITIAL_SOLUTION = 7,
	ACCEPTED_FOR_TESTING = 8,
	IGNORED = 9,
	DISQUALIFIED = 10,
	PENDING_CHECK = 11,
	MEMORY_LIMIT_EXCEEDED = 12,
	SECURITY_VIOLATION = 13,
	CODING_STYLE_VIOLATION = 14,
	WALL_TIME_LIMIT_EXCEEDED = 15,
	PENDING_REVIEW = 16,
	REJECTED = 17,
};

enum type {
	BOOL,
	STRING,
	INT,
	UINT,
};

/* NOTE: Read only %s from format. */
ssize_t tls_safe_read(struct tls *, void *, size_t);
ssize_t tls_safe_write(struct tls *, const void *, size_t);

void make_post_request(char *, ssize_t *, const char *, const char *,
    const size_t, const char *, ...);
void unpack_header(char *, char **, char **);
int unparse_json_field(const char *, const char *, enum type, void *);

const char *get_problem_status_name(const enum problem_status);
