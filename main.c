#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include "config.h"

/* file_path: lenght must equal path_lenght. */
void get_last_modify_file(const char *dir_path, const uint8_t level,
                          char *file_path, time_t *file_mtime);
void totemp_file(char *temp_path, const char *template_path);
void pre_send(const char *path);
static void send();

void
get_last_modify_file(const char *dir_path, const uint8_t level,
                     char *file_path, time_t *file_mtime)
{
	DIR *dir;
	struct dirent *entry;
	struct stat file_stat;
	char path[path_lenght];

	dir = opendir(dir_path);
	if (dir == NULL){
		perror("opendir");
		return;
	}

	while((entry = readdir(dir))){
		if (strcmp(entry->d_name, "..") == 0 || 
			strcmp(entry->d_name, ".") == 0) {
			continue;
		}

		if (sprintf(path, "%s/%s", dir_path, entry->d_name) >= sizeof(path)) {
			fprintf(stderr, "Real path longer than maximum lenght - %u.\n", path_lenght);
			continue;
		}

		switch(entry->d_type) {
		case DT_DIR:
			if (level < max_dir_walk_level) { 
				get_last_modify_file(path, level + 1, file_path, file_mtime);
			}
			break;
		case DT_REG:
			if (stat(path, &file_stat) == -1) {
				perror("stat");
				continue;
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
	}
}

void totemp_file(char *temp_path, const char *template_path)
{
	char buf[32];
	int fd_w, fd_r;
	int8_t lenght;

	fd_r = open(template_path, O_RDWR);
	if(fd_r == -1){
		perror("open");
		exit(EXIT_FAILURE);
	}
	fd_w = mkstemp(temp_path);
	if(fd_w == -1){
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}
	
	while((lenght = read(fd_r, buf, sizeof(buf))) != 0){
		if(write(fd_w, buf, lenght) == -1){
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	close(fd_w);
	close(fd_r);
}

void pre_send(const char *path)
{
	int status;
	const char *const *p;
	
	for(p = pre_send_actions; *p;){
		switch(fork()){
		case 0:
			execvp(*p, (char *const *)p);
			break;
		case -1:
			// TODO: Fail.
			break;
		}
		wait(&status);
		// TODO: Check status.

		while(*p++);
	}
}

void send()
{
	time_t file_mtime = 0;
	char file_path[path_lenght];

	get_last_modify_file((char *)start_dir, 0, file_path, &file_mtime);
	totemp_file(temp_path, file_path);
	pre_send(temp_path);
	
	printf("%s", temp_path);
	unlink(temp_path);
}

int
main(int argc, char *argv[])
{
	send();

	return 0;
}
