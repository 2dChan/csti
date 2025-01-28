/* 
 * See LICENSE file for copyright and license details. 
 */
#include <tls.h>

#include <sys/stat.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "networking.h"
#include "util.h"

#define HOST "unicorn.ejudge.ru"
#define BUFFER_SIZE 1024
#define SID_LENGHT 17
#define MESSAGE_LENGHT 100
#define AUTH_DATA_LENGHT 53
#define SEND_DATA_LENGHT 61

static int           auth(struct tls *, const char *, const char *,
                          const char *, char *, char *);
static struct tls    *connect(void);
static void          unpack_header(char *, char **, char **);

int 
auth(struct tls *ctx, const char *login, const char *password,
     const char *contest_id, char *sid, char *ejsid)
{
	static const char trequest[] = 
	    "POST /cgi-bin/new-client HTTP/1.1\r\n"
	    "Host: " HOST "\r\n"
	    "Conncetion: keep-alive\r\n"
	    "Content-Type: application/x-www-form-urlencoded\r\n"
	    "Content-Length: %ld\r\n"
	    "\r\n"
	    "action=login-json&login=%s&password=%s&contest_id=%s&json=1";

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	ssize_t lenght;
	int is_ok;

	lenght = AUTH_DATA_LENGHT + strlen(login) + strlen(password) + strlen(contest_id);
	snprintf(buffer, sizeof(buffer), trequest, lenght, login, password, contest_id);
	if (safe_tls_write(ctx, buffer, strlen(buffer)) == -1)
		return 1;

	lenght = safe_tls_read(ctx, buffer, sizeof(buffer));
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
nsubmit_run(const char *login, const char *password, const char *path, 
            char *header) 
{
	static const char trequest[] = 
		"POST /cgi-bin/new-client HTTP/1.1\r\n"
	    "Host: " HOST "\r\n"
		"Content-Type: multipart/form-data\r\n"
	    "Content-Length: %ld\r\n"
		"Conncetion: close\r\n\r\n"
		"action=submit-run&SID=%s&EJSID=%s&prob_id=%s&lang_id=%s&json=1&file=";

	struct tls *ctx;
	char sid[SID_LENGHT], ejsid[SID_LENGHT], *contest_id, *prob_id, lang_id[] = "1";

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	ssize_t lenght;
	struct stat fstat;
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
	
	lenght = SEND_DATA_LENGHT + strlen(sid) + strlen(ejsid) + strlen(prob_id) +
		strlen(lang_id);
	snprintf(buffer, BUFFER_SIZE, trequest, lenght, sid, ejsid, prob_id, lang_id);

	if (safe_tls_write(ctx, buffer, strlen(buffer)) == -1)
		goto failure;

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

	lenght = safe_tls_read(ctx, buffer, sizeof(buffer));
	if (lenght == -1)
		goto failure;
	buffer[lenght] = 0;

	unparse_json_field(buffer, "ok", BOOL, &is_ok);
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

