#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

static char temp_path[] = "/tmp/tmp.XXXXXX";

static const char start_dir[] = "ejudge";
static const char file_pattern[] = "*.c";
static const uint8_t max_dir_walk_level = 3;
static const uint16_t path_lenght = PATH_MAX;

static const char *const pre_send_actions[] = {
	"clang-format", "-i", temp_path, NULL,
	"cat", temp_path, NULL,
	NULL
};
