#ifndef OPENCLI_COMPILER_UTILS_H
#define OPENCLI_COMPILER_UTILS_H

#include <stdbool.h>

void set_compiler_verbose_logging(bool verbose);
bool init_compiler_dir(void);
bool is_compiler_installed(const char *version);
char *get_compiler_path(const char *version);
bool install_compiler(const char *version);
const char *get_appdata_path(void);

#endif /* OPENCLI_COMPILER_UTILS_H */ 