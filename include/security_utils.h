#ifndef OPENCLI_SECURITY_UTILS_H
#define OPENCLI_SECURITY_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_SAFE_PATH_LENGTH 1024
#define MAX_SAFE_FILENAME_LENGTH 256

/**
 * Validate and sanitize file path to prevent traversal attacks
 */
bool validate_safe_path(const char *path, char *safe_path, size_t safe_path_size);

/**
 * Check if filename contains only safe characters
 */
bool is_safe_filename(const char *filename);

/**
 * Validate file extension against whitelist
 */
bool validate_file_extension(const char *filepath, const char *allowed_extensions[], size_t ext_count);

/**
 * Sanitize command line argument
 */
bool sanitize_argument(const char *input, char *output, size_t output_size);

/**
 * Check if path is within allowed directory
 */
bool is_path_within_directory(const char *path, const char *base_dir);


#endif /* OPENCLI_SECURITY_UTILS_H */
