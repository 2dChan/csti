#include <stdio.h>

#include "networking.h"

int nsend_task(char *path) {
	int lenght;
	FILE *file;
	char buf[32];

	file = fopen(path, "r");
	if(file == NULL) {
		return 1;
	}

	while ((lenght = fread(buf, sizeof(*buf), sizeof(buf), file)) > 0) {
		if(fwrite(buf, sizeof(*buf), lenght, stdout) == -1){
			goto safe_error_exit;
		}
	}

	fclose(file);

	return 0;

safe_error_exit:
	fclose(file);
	return 1;
}
