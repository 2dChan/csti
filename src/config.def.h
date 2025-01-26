static const char login[] = "";
static const char password[] = "";

static const char file_pattern[] = "*";
static const char start_dir[] = ".";

static const char *pre_send_actions[] = {
	"clang-format", "-i", "path", NULL,
	NULL,
};
