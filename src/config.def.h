static const char start_dir[] = ".";
static const char file_pattern[] = "*.c";
static const uint8_t max_dir_walk_level = 3;
static const uint16_t path_lenght = PATH_MAX;

static const char *pre_send_actions[] = {
	"clang-format", "-i", "path", NULL,
	"echo", "path", NULL,
	NULL
};
