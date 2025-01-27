#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include "config.h"
#include "networking.h"

#define HEADER_SIZE 10
#define BUF_SIZE 1024

static int  apply_pre_send_actions(const char *);
static char *get_file_header(const char *);
static void get_last_modify_file(const char *, char *, time_t *);
static void submit();
static void totemp_file(char *, const char *);

int 
apply_pre_send_actions(const char *path)
{
	int status;
	uint8_t i;
	const char **p;
	
	for (p = pre_send_actions; *p;) {
		for (i = 0; p[i] != NULL; ++i) {
			if (strcmp(p[i], "path") == 0) {
				p[i] = path;
			}
		}

		switch(fork()) {
		case -1:
			perror("fork");
			return 1;
		case 0:
			execvp(*p, (char *const *)p);
			perror("execvp");
			_exit(EXIT_FAILURE);
		}

		wait(&status);
		if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "Action %s failed.\n", *p);
			return 1;
		}

		p += i + 1;
	}

	return 0;
}

char *
get_file_header(const char *file_path)
{
	FILE *file;
	char *header;

	file = fopen(file_path, "r");
	if (file == NULL) {
		perror("fopen");
		return NULL;
	}

	header = malloc(HEADER_SIZE);
	if (header == NULL) {
		perror("malloc");
		return NULL;
	}

	if (fgets(header, HEADER_SIZE, file) == NULL) {
		perror("fgets");
		return NULL;
	}

	return header;
}

void
get_last_modify_file(const char *dir_path, char *file_path, time_t *file_mtime)
{
	/* TODO: Add file header checker. */
	char path[PATH_MAX];
	struct stat file_stat;
	struct dirent *entry;
	DIR *dir;

	dir = opendir(dir_path);
	if (dir == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
		switch (entry->d_type) {
		case DT_DIR:
			get_last_modify_file(path, file_path, file_mtime);
			break;
		case DT_REG:
			if (stat(path, &file_stat) == -1) {
				perror("stat");
				exit(EXIT_FAILURE);
			}
			if (file_stat.st_mtime >= *file_mtime && 
				    fnmatch(file_pattern, entry->d_name, 0) == 0) {
				*file_mtime = file_stat.st_mtime;
				strcpy(file_path, path);
			}
			break;
		}
	}

	if (closedir(dir) == -1) {
		perror("closedir");
		exit(EXIT_FAILURE);
	}
}

void 
submit()
{
	char file_path[PATH_MAX], temp_path[] = "/tmp/tmp.XXXXXX", *header;
	time_t file_mtime = 0;

	get_last_modify_file(start_dir, file_path, &file_mtime);
	header = get_file_header(file_path);
	if (header == NULL) 
		exit(EXIT_FAILURE);

	totemp_file(temp_path, file_path);
	if (apply_pre_send_actions(temp_path))
		goto failure_exit;
	if (nsend_task(temp_path))
		goto failure_exit;

	free(header);
	unlink(temp_path);
	return;
	
failure_exit:
	free(header);
	unlink(temp_path);
	exit(EXIT_FAILURE);
}

void
totemp_file(char *temp_path, const char *template_path)
{
	char buf[BUF_SIZE];
	int fd_w, fd_r;
	int8_t lenght;

	fd_r = open(template_path, O_RDWR);
	if (fd_r == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	fd_w = mkstemp(temp_path);
	if (fd_w == -1) {
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}
	
	while ((lenght = read(fd_r, buf, sizeof(buf))) != 0) {
		if (write(fd_w, buf, lenght) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	close(fd_w);
	close(fd_r);
}

int
main(int argc, char *argv[])
{
	int let;

	if (argc == 1) {
		submit();
	}
	else {
		while ((let = getopt(argc, argv, "hv")) != -1) {
			switch (let) {
			// TODO: Move printf + exit to function.
			case 'v':
				printf("%s %s\n", argv[0], VERSION);
				exit(1);
			
			default:
				printf("Usage: %s [-v]\n", argv[0]);
				exit(1);
			}
		}
	}

	return 0;
}
