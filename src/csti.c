/*
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "networking.h"

#define HEADER_SIZE 20
#define BUF_SIZE	1024

static int apply_pre_send_actions(const char *);
static char *get_file_header(const char *);
static void get_last_modify_file(const char *, char *, time_t *);
static void get_status(void);
static void submit(void);
static void totemp(char *, const char *);

int
apply_pre_send_actions(const char *path)
{
	char cmd[BUF_SIZE], *wptr;
	const char pw[] = "path", *ptr;
	const size_t l = sizeof(pre_send_actions) / sizeof(*pre_send_actions),
				 pl = strlen(path), pwl = sizeof(pw) - 1;
	size_t i;
	int status;

	for (i = 0; i < l; ++i) {
		ptr = pre_send_actions[i];
		wptr = cmd;
		while (*ptr) {
			if (strncmp(ptr, pw, pwl) == 0) {
				sprintf(wptr, "%s", path);
				wptr += pl;
				ptr += pwl;
			}
			*wptr++ = *ptr++;
		}
		*wptr = 0;

		status = system(cmd);
		if (status == -1) {
			perror("status");
			return 1;
		}
		if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "apply_pre_send_action: Action %s failed\n", cmd);
			return 1;
		}
	}

	return 0;
}

char *
get_file_header(const char *path)
{
	FILE *f;
	char *h;

	f = fopen(path, "r");
	if (f == NULL) {
		perror("fopen");
		return NULL;
	}

	h = malloc(HEADER_SIZE);
	if (h == NULL) {
		perror("malloc");
		return NULL;
	}

	if (fgets(h, HEADER_SIZE, f) == NULL) {
		perror("fgets");
		return NULL;
	}

	return h;
}

void
get_last_modify_file(const char *dir_path, char *file_path, time_t *file_mtime)
{
	char path[PATH_MAX];
	struct stat s;
	struct dirent *ent;
	DIR *d;

	d = opendir(dir_path);
	if (d == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	while ((ent = readdir(d))) {
		if (strcmp(ent->d_name, "..") == 0 || strcmp(ent->d_name, ".") == 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", dir_path, ent->d_name);
		switch (ent->d_type) {
		case DT_DIR:
			get_last_modify_file(path, file_path, file_mtime);
			break;
		case DT_REG:
			if (stat(path, &s) == -1) {
				perror("stat");
				exit(EXIT_FAILURE);
			}
			if (s.st_mtime >= *file_mtime &&
				fnmatch(file_pattern, ent->d_name, 0) == 0) {
				*file_mtime = s.st_mtime;
				strcpy(file_path, path);
			}
			break;
		}
	}

	if (closedir(d) == -1) {
		perror("closedir");
		exit(EXIT_FAILURE);
	}
}

void
get_status(void)
{
	char fp[PATH_MAX], *h;
	time_t mtime = 0;

	get_last_modify_file(start_dir, fp, &mtime);
	if (mtime == 0) {
		fprintf(stderr, "get_last_modify_file: Can't find file\n");
		exit(EXIT_FAILURE);
	}

	h = get_file_header(fp);
	if (h == NULL)
		exit(EXIT_FAILURE);

	print_status_run(host, login, password, h);
	free(h);
}

void
submit(void)
{
	char fp[PATH_MAX], temp_path[] = "/tmp/tmp.XXXXXX", *h;
	time_t mtime = 0;

	get_last_modify_file(start_dir, fp, &mtime);
	if (mtime == 0) {
		fprintf(stderr, "get_last_modify_file: Can't find file\n");
		exit(EXIT_FAILURE);
	}

	h = get_file_header(fp);
	if (h == NULL)
		exit(EXIT_FAILURE);

	totemp(temp_path, fp);
	if (apply_pre_send_actions(temp_path))
		goto failure_exit;
	if (submit_run(host, login, password, temp_path, h))
		goto failure_exit;

	free(h);
	unlink(temp_path);
	return;

failure_exit:
	free(h);
	unlink(temp_path);
	exit(EXIT_FAILURE);
}

void
totemp(char *temp_path, const char *template_path)
{
	char buf[BUF_SIZE];
	ssize_t l;
	int fdw, fdr;

	fdr = open(template_path, O_RDWR);
	if (fdr == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	fdw = mkstemp(temp_path);
	if (fdw == -1) {
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}

	while ((l = read(fdr, buf, sizeof(buf))) != 0 && l != -1) {
		if (write(fdw, buf, l) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
	if (l == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	close(fdw);
	close(fdr);
}

int
main(int argc, char *argv[])
{
	int let;

	if (argc == 1) {
		submit();
	} else {
		while ((let = getopt(argc, argv, "svh")) != -1) {
			switch (let) {
			case 's':
				get_status();
				break;
			case 'v':
#ifdef VERSION
				printf("%s %s\n", argv[0], VERSION);
#else
				printf("%s\n", argv[0]);
#endif
				exit(EXIT_FAILURE);
			default:
				printf("Usage: %s [-v] [-s]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
	}

	return 0;
}
