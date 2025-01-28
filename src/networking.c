/* 
 * See LICENSE file for copyright and license details. 
 */
#include <tls.h>

#include <sys/stat.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "networking.h"
#include "util.h"

#define HOST "unicorn.ejudge.ru"
#define BUFFER_SIZE 1024
#define SID_LENGHT 17
#define MESSAGE_LENGHT 100

static int           auth(struct tls *, const char *, const char *,
                          const char *, char *, char *);
static struct tls    *connect(void);
static void          unpack_header(char *, char **, char **);

int 
auth(struct tls *ctx, const char *login, const char *password,
     const char *contest_id, char *sid, char *ejsid)
{
	static const char content_type[] = "application/x-www-form-urlencoded",
	data_template[] = 
		"action=login-json&login=%s&password=%s&contest_id=%s&json=1";

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT], *request;
	ssize_t lenght = 0;
	int is_ok;

	request = make_post_request(HOST, content_type, 0, data_template, login, 
	                            password, contest_id);
	if (request == NULL)
		return 1;

	if (safe_tls_write(ctx, request, strlen(request)) == -1) {
		free(request);
		return 1;
	}

	free(request);

	lenght = safe_tls_read(ctx, buffer, sizeof(buffer) - 1);
	if (lenght == -1) 
		return 1;
	buffer[lenght] = 0;

	if (unparse_json_field(buffer, "ok", BOOL, &is_ok))
		return 1;

	if (is_ok == 0) {
		if (unparse_json_field(buffer, "message", STRING, &message))
			return 1;
		fprintf(stderr, "ejudge error: %s\n", message);
		return 1;
	}

	if (unparse_json_field(buffer, "SID", STRING, sid) ||
       unparse_json_field(buffer, "EJSID", STRING, ejsid))
		return 1;
	
	return 0;
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
			}
			else if (*prob_id == NULL) {
				*prob_id = ptr + 1;
			}
			else {
				break;
			}
		}
	}
}

struct tls *
connect(void)
{
	/* 
	 * TODO: 
	 * 1) Error handle.
	 * 2) Setup config.
	 */

	struct tls_config *config;
    struct tls *ctx;

    config = tls_config_new();
	ctx = tls_client();
    tls_configure(ctx, config);
	tls_connect(ctx, HOST, "443");

	return ctx;
}

int 
submit_run(const char *login, const char *password, const char *path, 
            char *header) 
{
	/*
	 * TODO: Unparse lang_id from action=problem-status-json.
	 */
	static const char content_type[] = "multipart/form-data",
	data_template[] = 
		"action=submit-run&json=1&SID=%s&EJSID=%s&prob_id=%s&lang_id=%s&file=";

	char sid[SID_LENGHT], ejsid[SID_LENGHT], *contest_id, *prob_id, lang_id[] = "2";
	char *request, buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	struct stat fstat;
	struct tls *ctx;
	ssize_t lenght;
	int fd, is_ok;

	ctx = connect();
	if (ctx == NULL)
		return 1;

	unpack_header(header, &contest_id, &prob_id);
	if (contest_id == NULL || prob_id == NULL) {
		fprintf(stderr, "unpack_header: Header presentation error\n");
		return 1;
	}
	
	if (auth(ctx, login, password, contest_id, sid, ejsid))
		goto failure;

	if (stat(path, &fstat) == -1)
		goto failure;

	request = make_post_request(HOST, "multipart/form-data", fstat.st_size, 
	                            data_template, sid, ejsid, prob_id, lang_id);
	if(request == NULL)
		goto failure;

	if (safe_tls_write(ctx, request, strlen(request)) == -1) {
		free(request);
		goto failure;
	}

	free(request);

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		goto failure;
	}

	while ((lenght = read(fd, buffer, sizeof(buffer))) != 0) {
		buffer[lenght] = 0;
		if (safe_tls_write(ctx, buffer, lenght) == -1)
			goto failure;
	}
	close(fd);

	lenght = safe_tls_read(ctx, buffer, sizeof(buffer) - 1);
	if (lenght == -1)
		goto failure;
	buffer[lenght] = 0;

	if (unparse_json_field(buffer, "ok", BOOL, &is_ok))
		goto failure;
	if (is_ok == 0) {
		if (unparse_json_field(buffer, "message", STRING, &message))
			return 1;
		fprintf(stderr, "ejudge error: %s\n", message);
		return 1;
	}

	/* TODO: Unwrap request. */
	puts(buffer);

    tls_close(ctx);
	tls_free(ctx);
	return 0;

failure:
	tls_close(ctx);
	tls_free(ctx);
	return 1;
}

