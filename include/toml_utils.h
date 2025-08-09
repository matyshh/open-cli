#ifndef OPENCLI_TOML_UTILS_H
#define OPENCLI_TOML_UTILS_H

#include <stdbool.h>

char *read_toml_entry_file(const char *toml_path);
char *read_toml_output_file(const char *toml_path);
char *read_toml_compiler_version(const char *toml_path);

// Returns a NULL-terminated array of strings that must be freed by caller
char **read_toml_include_paths(const char *toml_path, int *count);

// Returns a NULL-terminated array of strings that must be freed by caller
char **read_toml_compiler_args(const char *toml_path, int *count);

// Get the directory part of a file path
char *get_directory_path(const char *file_path);

// Get the relative include path for quoted includes
char *get_relative_include_path(const char *base_file, const char *include_file);

#endif /* OPENCLI_TOML_UTILS_H */