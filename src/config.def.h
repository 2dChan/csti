static const char host[] = "unicorn.ejudge.ru";
static const char lang_id[] = "2"; /* C - 2/CPP - 3/NASM - 50 */
static const char login[] = "";
static const char password[] = "";

static const char file_pattern[] = "*"; /* "*" - wildcard.  */
static const char start_dir[] = ".";    /* "." - curret directory. */

/* 
 * NOTE: 
 * 1) Replace "path" to file path.
 * 2) Set ":" if array size zero.
 */
static const char *pre_send_actions[] = {
	"sed -i '/cut/,/cut/d' path",
};

