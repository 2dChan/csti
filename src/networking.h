/*
 * See LICENSE file for copyright and license details.
 */

static const char header_pattern[] =
	"[[:punct:]]{0,3}[[:blank:]]*([0-9]+)-([0-9]+)";

void print_status_run(const char *, const char *, const char *, char *);
int submit_run(const char *, const char *, const char *, const char *, char *);
