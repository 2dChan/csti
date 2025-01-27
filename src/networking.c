// 521-1	
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <tls.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>


#include "config.h"
#include "networking.h"
#include "util.h"

enum {
	AUTH_REQUEST_DATA_LENGHT=46,
	SEND_REQUEST_DATA_LENGHT=61,
	BUFFER_SIZE=1024,
	SID_LENGHT=17,
	ID_MAX_LEN=5,
};

static const char host[] = "unicorn.ejudge.ru";

static void auth(struct tls *ctx, char *contest_id, char *sid, char *ejsid);
static struct tls *connect(void);

void 
auth(struct tls *ctx, char *contest_id, char *sid, char *ejsid)
{
	/* TODO: Error handle */
	static const char trequest[] = 
	    "POST /cgi-bin/new-client HTTP/1.1\r\n"
	    "Host: %s\r\n"
	    "Conncetion: keep-alive\r\n"
	    "Content-Type: application/x-www-form-urlencoded\r\n"
	    "Content-Length: %ld\r\n"
	    "\r\n"
	    "action=login-json&login=%s&password=%s&contest_id=%s";
	char buffer[BUFFER_SIZE];
	ssize_t data_lenght;

	data_lenght = AUTH_REQUEST_DATA_LENGHT + strlen(login) +
		          strlen(password) + strlen(contest_id);
	snprintf(buffer, sizeof(buffer), trequest, host,
	         data_lenght, login, password, contest_id);

	safe_tls_write(ctx, buffer, strlen(buffer));
	bzero(buffer, sizeof(buffer));
	safe_tls_read(ctx, buffer, sizeof(buffer));
	
	unparse_json_field(buffer, "SID", sid, SID_LENGHT - 1);
	unparse_json_field(buffer, "EJSID", ejsid, SID_LENGHT - 1);
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
	tls_connect(ctx, host, "443");

	return ctx;
}


int 
nsend_task(char *path) 
{
	char contest_id[ID_MAX_LEN], prob_id[ID_MAX_LEN], lang_id[] = "1";
	FILE* file = fopen(path, "r");
	char *ptr;
	int flag = 1;
	while(flag != 0) {
		int let = fgetc(file);
		switch (let) {
		case '/':
		case ' ':
			ptr = contest_id;
			continue;
		case '-':
			ptr = 0;
			fscanf(file, "%s", prob_id);
			flag = 0;
			break;
		default:
			*ptr = let;
			ptr++;
		}
	}
	fclose(file);
	/* TODO: Error handle */

	const char request[] = 
		"POST /cgi-bin/new-client HTTP/1.1\r\n"
		"Host: unicorn.ejudge.ru\r\n"
		"Content-Type: multipart/form-data\r\n"
	    "Content-Length: %ld\r\n"
		"Conncetion: close\r\n\r\n"
		"action=submit-run&SID=%s&EJSID=%s&prob_id=%s&lang_id=%s&json=1&file=";
	char buffer[BUFFER_SIZE] = { 0 }, sid[SID_LENGHT], ejsid[SID_LENGHT];
	ssize_t lenght;
	int fd;
	struct stat fstat;
	struct tls *ctx;

	ctx = connect();
	auth(ctx, contest_id, sid, ejsid);
	
	if (stat(path, &fstat) == -1){
		goto failure;
	}
	lenght = SEND_REQUEST_DATA_LENGHT + strlen(sid) +
	    strlen(ejsid) + strlen(prob_id) + strlen(lang_id);
	snprintf(buffer, BUFFER_SIZE, request, lenght, sid, ejsid, prob_id, lang_id);
	printf("%s\n", buffer);
	fflush(stdout);
	safe_tls_write(ctx, buffer, strlen(buffer));
	fd = open(path, O_RDONLY);
	while((lenght = read(fd, buffer, sizeof(buffer))) != 0) {
		safe_tls_write(ctx, buffer, lenght);
		bzero(buffer, sizeof(buffer));
	}
	close(fd);
	printf("%s\n", buffer);
	fflush(stdout);

	safe_tls_read(ctx, buffer, sizeof(buffer));
	printf("%s\n", buffer);
	
    tls_close(ctx);
    tls_free(ctx);
	return 0;
failure:
	tls_close(ctx);
    tls_free(ctx);
	return 1;
}

int 
nget_task_status(char *path) 
{
	
	return 0;
}

