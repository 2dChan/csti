/*
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tls.h>
#include <unistd.h>

#include "networking.h"
#include "util.h"

#define BUFFER_SIZE	   4096
#define SID_LENGHT	   17
#define MESSAGE_LENGHT 100

struct problem_run {
	unsigned int id;
	unsigned int passed_tests;
	unsigned int prob_id;
	unsigned int score;
	unsigned int status;
};

struct problem_info {
	unsigned int lang_id;
};

static int auth(struct tls *, const char *, const char *, const char *,
	const char *, char *, char *);
static struct tls *connect(const char *);
static int get_problem_info(struct problem_info *, struct tls *, const char *,
	const char *, const char *, const char *);
static int get_problem_runs(struct problem_run *, unsigned int, struct tls *,
	const char *, const char *, const char *, const char *);

int
auth(struct tls *ctx, const char *host, const char *login, const char *password,
	const char *contest_id, char *sid, char *ejsid)
{
	static const char
		content_type[] = "application/x-www-form-urlencoded",
		data_template[] =
			"action=login-json&login=%s&password=%s&contest_id=%s&json=1";

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	ssize_t lenght = 0;
	int is_ok;

	lenght = make_post_request(buffer, host, content_type, 0, data_template,
		login, password, contest_id);
	if (lenght > sizeof(buffer) || lenght == -1) {
		fprintf(stderr, "make_post_request error: Lenght %ld", lenght);
		return 1;
	}
	if (tls_safe_write(ctx, buffer, lenght) == -1)
		return 1;

	lenght = tls_safe_read(ctx, buffer, sizeof(buffer) - 1);
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

	// Clear socket fd.
	while (lenght + 1 == BUFFER_SIZE && lenght != 0) {
		lenght = tls_safe_read(ctx, buffer, BUFFER_SIZE);
		if (lenght == -1)
			return -1;
	}
	return 0;
}

struct tls *
connect(const char *host)
{
	struct tls_config *config;
	struct tls *ctx;

	config = tls_config_new();
	if (config == NULL) {
		printf("tls_config_new: %s\n", tls_config_error(config));
		return NULL;
	}
	ctx = tls_client();
	if (ctx == NULL) {
		printf("tls_client: %s\n", tls_error(ctx));
		return NULL;
	}
	if (tls_configure(ctx, config) == -1) {
		printf("tls_configure: %s\n", tls_error(ctx));
		return NULL;
	}
	if (tls_connect(ctx, host, "443") == -1) {
		printf("tls_connect: %s\n", tls_error(ctx));
		return NULL;
	}

	return ctx;
}

int
get_problem_info(struct problem_info *problem_info, struct tls *ctx,
	const char *host, const char *sid, const char *ejsid, const char *problem)
{
	static const char problem_runs_request[] = GET_REQUEST_TEMPLATE(
		"action=problem-status-json&problem=%s");

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	size_t lenght;
	int is_ok;

	lenght = sprintf(buffer, problem_runs_request, sid, ejsid, problem, host);
	if (lenght > sizeof(buffer))
		return 1;
	if (tls_safe_write(ctx, buffer, lenght) == -1)
		return 1;

	lenght = tls_safe_read(ctx, buffer, sizeof(buffer) - 1);
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
	/* NOTE: Unparse array as INT maybe not stable, and get only first num. */
	if (unparse_json_field(buffer, "compilers", INT, &problem_info->lang_id))
		return 1;

	// Clear socket fd.
	while (lenght + 1 == BUFFER_SIZE && lenght != 0) {
		lenght = tls_safe_read(ctx, buffer, BUFFER_SIZE);
		if (lenght == -1)
			return -1;
	}

	return 0;
}

int
get_problem_runs(struct problem_run *runs, unsigned int n, struct tls *ctx,
	const char *host, const char *sid, const char *ejsid, const char *problem)
{
	static const char problem_runs_request[] = GET_REQUEST_TEMPLATE(
		"action=list-runs-json&prob_id=%s");

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT];
	size_t lenght, i;
	int is_ok, status;

	lenght = sprintf(buffer, problem_runs_request, sid, ejsid, problem, host);
	if (lenght > sizeof(buffer)) {
		return -1;
	}
	if (tls_safe_write(ctx, buffer, lenght) == -1) {
		return -1;
	}

	lenght = tls_safe_read(ctx, buffer, sizeof(buffer) - 1);
	if (lenght == -1)
		return -1;
	buffer[lenght] = 0;

	if (unparse_json_field(buffer, "ok", BOOL, &is_ok))
		return -1;
	if (is_ok == 0) {
		if (unparse_json_field(buffer, "message", STRING, &message))
			return -1;
		fprintf(stderr, "ejudge error: %s\n", message);
		return -1;
	}
	for (i = 0; i < n; ++i) {
		status = unparse_json_field(buffer, "run_id", UINT, &runs[i].id);
		if (status == 2) {
			if (lenght + 1 == BUFFER_SIZE) {
				lenght = tls_safe_read(ctx, buffer, BUFFER_SIZE);
				if (lenght == -1)
					return -2;
				buffer[lenght] = 0;
			} else
				break;
		}

		if (status ||
			unparse_json_field(buffer, "prob_id", UINT, &runs[i].prob_id) ||
			unparse_json_field(buffer, "status", UINT, &runs[i].status)) {
			return -1;
		}
		if (runs[i].status == PARITIAL_SOLUTION || runs[i].status == OK) {
			if (unparse_json_field(buffer, "passed_tests", UINT,
					&runs[i].passed_tests) == 1 ||
				unparse_json_field(buffer, "score", UINT, &runs[i].score) ==
					1) {
				return -1;
			}
		} else {
			runs[i].passed_tests = 0;
			runs[i].score = 0;
		}
	}

	// Clear socket fd.
	while (lenght + 1 == BUFFER_SIZE && lenght != 0) {
		lenght = tls_safe_read(ctx, buffer, BUFFER_SIZE);
		if (lenght == -1)
			return -1;
	}

	return i;
}

void
print_status_run(const char *host, const char *login, const char *password,
	char *header)
{
	char sid[SID_LENGHT], ejsid[SID_LENGHT], *contest_id, *prob_id;
	const char *status;
	struct problem_run problem_run;
	struct tls *ctx;
	ssize_t lenght;

	unpack_header(header, &contest_id, &prob_id);
	if (contest_id == NULL || prob_id == NULL) {
		fprintf(stderr, "unpack_header: Header presentation error\n");
		exit(EXIT_FAILURE);
	}

	ctx = connect(host);
	if (ctx == NULL)
		exit(EXIT_FAILURE);
	if (auth(ctx, host, login, password, contest_id, sid, ejsid))
		exit(EXIT_FAILURE);

	lenght = get_problem_runs(&problem_run, 1, ctx, host, sid, ejsid, prob_id);
	if (lenght == -1)
		exit(EXIT_FAILURE);
	if (lenght == -2) {
		printf("No solutions found");
		return;
	}

	status = get_problem_status_name(problem_run.status);
	printf("Prob id |Run id |%-*s |Tests |Score \n"
		   "%-8u|%-7u|%-6s |%-6u|%-6u\n",
		(int)strlen(status), "Status", problem_run.prob_id, problem_run.id,
		status, problem_run.passed_tests, problem_run.score);
}

int
submit_run(const char *host, const char *login, const char *password,
	const char *path, char *header)
{
	static const char
		content_type[] = "multipart/form-data",
		data_template[] =
			"action=submit-run&json=1&SID=%s&EJSID=%s&prob_id=%s&lang_id=%u&file=";

	char buffer[BUFFER_SIZE], message[MESSAGE_LENGHT], sid[SID_LENGHT],
		ejsid[SID_LENGHT], *contest_id, *prob_id;
	struct problem_info info;
	struct stat fstat;
	struct tls *ctx;
	ssize_t lenght;
	int fd, is_ok;

	unpack_header(header, &contest_id, &prob_id);
	if (contest_id == NULL || prob_id == NULL) {
		fprintf(stderr, "unpack_header: Header presentation error\n");
		return 1;
	}

	/* Connect. */
	ctx = connect(host);
	if (ctx == NULL)
		return 1;
	if (auth(ctx, host, login, password, contest_id, sid, ejsid))
		goto failure;

	/* Send. */
	if (get_problem_info(&info, ctx, host, sid, ejsid, prob_id)) {
		return 1;
	}

	if (stat(path, &fstat) == -1)
		goto failure;
	lenght = make_post_request(buffer, host, content_type, fstat.st_size,
		data_template, sid, ejsid, prob_id, info.lang_id);
	if (lenght > sizeof(buffer) || lenght == -1) {
		fprintf(stderr, "make_post_request error: Lenght %ld", lenght);
		goto failure;
	}
	if (tls_safe_write(ctx, buffer, lenght) == -1) {
		goto failure;
	}

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		goto failure;
	}
	while ((lenght = read(fd, buffer, sizeof(buffer))) != 0) {
		if (tls_safe_write(ctx, buffer, lenght) == -1) {
			close(fd);
			goto failure;
		}
	}
	close(fd);

	/* Recv. */
	lenght = tls_safe_read(ctx, buffer, sizeof(buffer) - 1);
	if (lenght == -1)
		goto failure;
	buffer[lenght] = 0;

	if (unparse_json_field(buffer, "ok", BOOL, &is_ok))
		goto failure;
	if (is_ok == 0) {
		if (unparse_json_field(buffer, "message", STRING, &message))
			goto failure;
		fprintf(stderr, "ejudge error: %s\n", message);
		goto failure;
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
