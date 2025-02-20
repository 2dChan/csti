/*
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tls.h>
#include <unistd.h>

#include "networking.h"
#include "util.h"

#define BUF_SIZE	   4096
#define SID_LENGHT	   17
#define MESSAGE_LENGHT 100

#define URI			   "/cgi-bin/new-client"

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
static const char *problem_status_to_str(const enum problem_status);
static int unpack_header(char *, char **, char **);

int
auth(struct tls *ctx, const char *host, const char *login, const char *password,
	const char *contest, char *sid, char *ejsid)
{
	static const char
		ct[] = "application/x-www-form-urlencoded",
		dt[] = "action=login-json&login=%s&password=%s&contest_id=%s&json=1";

	char buf[BUF_SIZE], msg[MESSAGE_LENGHT];
	ssize_t l = 0;
	int ok;

	l = make_post_request(buf, URI, host, ct, 0, dt, login, password, contest);
	if (l > sizeof(buf) || l == -1) {
		fprintf(stderr, "make_post_request: Lenght %ld/%ld", l, sizeof(buf));
		return 1;
	}
	if (tls_safe_write(ctx, buf, l) == -1)
		return 1;

	l = tls_safe_read(ctx, buf, sizeof(buf) - 1);
	if (l == -1)
		return 1;
	buf[l] = 0;

	if (unparse_json_field(buf, "ok", BOOL, &ok))
		return 1;
	if (ok == 0) {
		if (unparse_json_field(buf, "message", STRING, &msg))
			return 1;
		fprintf(stderr, "ejudge error: %s\n", msg);
		return 1;
	}
	if (unparse_json_field(buf, "SID", STRING, sid) ||
		unparse_json_field(buf, "EJSID", STRING, ejsid))
		return 1;

	while (l + 1 == BUF_SIZE && l != 0) {
		l = tls_safe_read(ctx, buf, BUF_SIZE);
		if (l == -1)
			return -1;
	}
	return 0;
}

struct tls *
connect(const char *host)
{
	struct tls_config *cfg;
	struct tls *ctx;

	cfg = tls_config_new();
	if (cfg == NULL) {
		printf("tls_config_new: %s\n", tls_config_error(cfg));
		return NULL;
	}
	ctx = tls_client();
	if (ctx == NULL) {
		printf("tls_client: %s\n", tls_error(ctx));
		return NULL;
	}
	if (tls_configure(ctx, cfg) == -1) {
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
get_problem_info(struct problem_info *pi, struct tls *ctx, const char *host,
	const char *sid, const char *ejsid, const char *problem)
{
	static const char rt[] = GET_REQUEST_TEMPLATE(
		"action=problem-status-json&problem=%s");

	char buf[BUF_SIZE], msg[MESSAGE_LENGHT];
	size_t l;
	int ok;

	l = sprintf(buf, rt, sid, ejsid, problem, host);
	if (l > sizeof(buf))
		return 1;
	if (tls_safe_write(ctx, buf, l) == -1)
		return 1;

	l = tls_safe_read(ctx, buf, sizeof(buf) - 1);
	if (l == -1)
		return 1;
	buf[l] = 0;

	if (unparse_json_field(buf, "ok", BOOL, &ok))
		return 1;
	if (ok == 0) {
		if (unparse_json_field(buf, "message", STRING, &msg))
			return 1;
		fprintf(stderr, "ejudge error: %s\n", msg);
		return 1;
	}
	/* NOTE: Unparse array as INT maybe not stable, and get only first num. */
	if (unparse_json_field(buf, "compilers", UINT, &pi->lang_id))
		return 1;

	while (l + 1 == BUF_SIZE && l != 0) {
		l = tls_safe_read(ctx, buf, BUF_SIZE);
		if (l == -1)
			return -1;
	}

	return 0;
}

int
get_problem_runs(struct problem_run *runs, unsigned int n, struct tls *ctx,
	const char *host, const char *sid, const char *ejsid, const char *problem)
{
	static const char rt[] = GET_REQUEST_TEMPLATE(
		"action=list-runs-json&prob_id=%s");

	char buf[BUF_SIZE], msg[MESSAGE_LENGHT];
	size_t l, i;
	int ok, status;

	l = sprintf(buf, rt, sid, ejsid, problem, host);
	if (l > sizeof(buf) || l == -1) {
		return -1;
	}
	if (tls_safe_write(ctx, buf, l) == -1) {
		return -1;
	}

	l = tls_safe_read(ctx, buf, sizeof(buf) - 1);
	if (l == -1)
		return -1;
	buf[l] = 0;

	if (unparse_json_field(buf, "ok", BOOL, &ok))
		return -1;
	if (ok == 0) {
		if (unparse_json_field(buf, "message", STRING, &msg))
			return -1;
		fprintf(stderr, "ejudge error: %s\n", msg);
		return -1;
	}
	for (i = 0; i < n; ++i) {
		status = unparse_json_field(buf, "run_id", UINT, &runs[i].id);
		// TODO: Refactor after rewrite get response.
		if (status == 2) {
			if (l + 1 == BUF_SIZE) {
				l = tls_safe_read(ctx, buf, BUF_SIZE);
				if (l == -1)
					return -1;
				buf[l] = 0;
			} else
				break;
		}

		if (status ||
			unparse_json_field(buf, "prob_id", UINT, &runs[i].prob_id) ||
			unparse_json_field(buf, "status", UINT, &runs[i].status)) {
			return -1;
		}
		if (runs[i].status == PARITIAL_SOLUTION || runs[i].status == OK) {
			if (unparse_json_field(buf, "passed_tests", UINT,
					&runs[i].passed_tests) == 1 ||
				unparse_json_field(buf, "score", UINT, &runs[i].score) == 1) {
				return -1;
			}
		} else {
			runs[i].passed_tests = 0;
			runs[i].score = 0;
		}
	}

	while (l + 1 == BUF_SIZE && l != 0) {
		l = tls_safe_read(ctx, buf, BUF_SIZE);
		if (l == -1)
			return -1;
	}

	return i;
}

const char *
problem_status_to_str(const enum problem_status s)
{
	switch (s) {
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

int
unpack_header(char *header, char **contest_id, char **prob_id)
{
	char msg[MESSAGE_LENGHT];
	regmatch_t m[3];
	regex_t r;
	int val;

	val = regcomp(&r, header_pattern, REG_EXTENDED);
	if (val != 0)
		goto re_failure;
	val = regexec(&r, header, 3, m, 0);
	switch (val) {
	case 0:
		*contest_id = header + m[1].rm_so;
		*prob_id = header + m[2].rm_so;
		header[m[1].rm_eo] = 0;
		header[m[2].rm_eo] = 0;
		return 0;
	case REG_NOMATCH:
		// TODO: Earlie was failure, fix error handling.
		goto re_failure;
	default:
		goto re_failure;
	}

re_failure:
	regerror(val, &r, msg, sizeof(msg));
	fprintf(stderr, "regex: %s\n", msg);
failure:
	regfree(&r);
	return 1;
}

void
print_status_run(const char *host, const char *login, const char *password,
	char *header)
{
	char sid[SID_LENGHT], ejsid[SID_LENGHT], *contest_id, *prob_id;
	const char *status;
	struct problem_run pr;
	struct tls *ctx;
	ssize_t lenght;

	if (unpack_header(header, &contest_id, &prob_id))
		exit(EXIT_FAILURE);

	ctx = connect(host);
	if (ctx == NULL)
		exit(EXIT_FAILURE);
	if (auth(ctx, host, login, password, contest_id, sid, ejsid))
		exit(EXIT_FAILURE);

	lenght = get_problem_runs(&pr, 1, ctx, host, sid, ejsid, prob_id);
	if (lenght == -1)
		exit(EXIT_FAILURE);
	if (lenght == 0) {
		printf("No solutions found\n");
		return;
	}

	status = problem_status_to_str(pr.status);
	printf("Prob id |Run id |%-*s |Tests |Score \n%-8u|%-7u|%-6s |%-6u|%-6u\n",
		(int)strlen(status), "Status", pr.prob_id, pr.id, status,
		pr.passed_tests, pr.score);
}

int
submit_run(const char *host, const char *login, const char *password,
	const char *path, char *header)
{
	static const char
		ct[] = "multipart/form-data; boundary=-------------573cf973d5228",
		dt[] = "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"action\"\r\n\r\n"
			   "submit-run\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"json\"\r\n\r\n"
			   "1\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"SID\"\r\n\r\n"
			   "%s\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"EJSID\"\r\n\r\n"
			   "%s\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"prob_id\"\r\n\r\n"
			   "%s\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"lang_id\"\r\n\r\n"
			   "%u\r\n"
			   "---------------573cf973d5228\r\n"
			   "Content-Disposition: form-data; name=\"file\"\r\n"
			   "Content-Type: text/plain; \r\n"
			   "\r\n",
		eb[] = "\r\n---------------573cf973d5228--\r\n";

	char buf[BUF_SIZE], msg[MESSAGE_LENGHT], sid[SID_LENGHT], ejsid[SID_LENGHT],
		*contest_id, *prob_id;
	struct problem_info pi;
	struct stat s;
	struct tls *ctx;
	ssize_t l;
	int fd, ok;

	if (unpack_header(header, &contest_id, &prob_id))
		return 1;

	ctx = connect(host);
	if (ctx == NULL)
		return 1;
	if (auth(ctx, host, login, password, contest_id, sid, ejsid))
		goto failure;

	printf("Con1\n");
	fflush(stdout);
	// if (get_problem_info(&pi, ctx, host, sid, ejsid, prob_id)) {
	// 	goto failure;
	// }
	pi.lang_id = 3;

	if (stat(path, &s) == -1)
		goto failure;

	l = make_post_request(buf, URI, host, ct, s.st_size + sizeof(eb), dt, sid,
		ejsid, prob_id, pi.lang_id);

	buf[l] = 0;
	printf("%ld %s\n", l, buf);
	fflush(stdout);

	if (l > sizeof(buf) || l == -1) {
		fprintf(stderr, "make_post_request error: Lenght %ld", l);
		goto failure;
	}
	if (tls_safe_write(ctx, buf, l) == -1) {
		goto failure;
	}

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		goto failure;
	}
	while ((l = read(fd, buf, sizeof(buf))) != 0) {
		if (tls_safe_write(ctx, buf, l) == -1) {
			close(fd);
			goto failure;
		}
		write(STDIN_FILENO, buf, l);
	}
	tls_safe_write(ctx, eb, sizeof(eb));
	puts(eb);
	close(fd);

	l = tls_safe_read(ctx, buf, sizeof(buf) - 1);
	if (l == -1)
		goto failure;
	buf[l] = 0;

	if (unparse_json_field(buf, "ok", BOOL, &ok))
		goto failure;
	if (ok == 0) {
		if (unparse_json_field(buf, "message", STRING, &msg))
			goto failure;
		fprintf(stderr, "ejudge error: %s\n", msg);
		goto failure;
	}

	/* TODO: Unwrap request. */
	puts(buf);

	tls_close(ctx);
	tls_free(ctx);
	return 0;

failure:
	tls_close(ctx);
	tls_free(ctx);
	return 1;
}
