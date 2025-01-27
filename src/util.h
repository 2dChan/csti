#include <stdio.h>

ssize_t safe_tls_read(struct tls *ctx, void *buf, size_t lenght);
ssize_t safe_tls_write(struct tls *ctx, const void *buf, size_t lenght);

void unparse_json_field(const char *json, const char *name, char *value, size_t lenght);
