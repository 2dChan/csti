static const char host[] = "unicorn.ejudge.ru";
static const char lang_id[] = "2";
static const char login[] = "";
static const char password[] = "";

static const char file_pattern[] = "*"; /* "*" - wildcard.  */
static const char start_dir[] = ".";    /* "." - curret directory. */

/* 
 * NOTE: 
 * 1) NULL ptr sign the end of the command.
 * 2) Two last NULL ptr sign the array end. 
 * 3) "path" replace to file path.
 */
static const char *pre_send_actions[] = {
	"clang-format", "-i", "path", NULL,
	NULL,
};
