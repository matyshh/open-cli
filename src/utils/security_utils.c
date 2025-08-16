#include "security_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#define strcasecmp _stricmp
#else
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#endif

bool is_safe_filename(const char *filename) {
    if (!filename || strlen(filename) == 0 || strlen(filename) > MAX_SAFE_FILENAME_LENGTH) {
        return false;
    }

    // Check for dangerous characters
    const char *dangerous_chars = "<>:\"|?*";
    if (strpbrk(filename, dangerous_chars) != NULL) {
        return false;
    }

    // Check for path traversal sequences
    if (strstr(filename, "..") != NULL) {
        return false;
    }

    // Check for control characters and ensure printable ASCII
    for (const char *p = filename; *p; p++) {
        if (*p < 32 || *p == 127) {
            return false;
        }
    }

    // Windows reserved names
    const char *reserved_names[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    char upper_name[MAX_SAFE_FILENAME_LENGTH];
    strncpy(upper_name, filename, sizeof(upper_name) - 1);
    upper_name[sizeof(upper_name) - 1] = '\0';
    
    for (char *p = upper_name; *p; p++) {
        *p = (char)toupper(*p);
    }

    for (size_t i = 0; i < sizeof(reserved_names) / sizeof(reserved_names[0]); i++) {
        if (strcmp(upper_name, reserved_names[i]) == 0) {
            return false;
        }
    }

    return true;
}

bool validate_file_extension(const char *filepath, const char *allowed_extensions[], size_t ext_count) {
    if (!filepath || !allowed_extensions || ext_count == 0) {
        return false;
    }

    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) {
        return false;
    }

    const char *extension = dot + 1;
    
    for (size_t i = 0; i < ext_count; i++) {
        if (strcasecmp(extension, allowed_extensions[i]) == 0) {
            return true;
        }
    }

    return false;
}

static bool normalize_path_internal(const char *input_path, char *normalized_path, size_t normalized_size) {
    if (!input_path || !normalized_path || normalized_size == 0) {
        return false;
    }

#ifdef _WIN32
    DWORD result = GetFullPathName(input_path, (DWORD)normalized_size, normalized_path, NULL);
    return (result > 0 && result < normalized_size);
#else
    char *resolved = realpath(input_path, NULL);
    if (!resolved) {
        // If realpath fails, try manual normalization
        char temp_path[PATH_MAX];
        strncpy(temp_path, input_path, sizeof(temp_path) - 1);
        temp_path[sizeof(temp_path) - 1] = '\0';
        
        char *components[256];
        int component_count = 0;
        
        char *token = strtok(temp_path, "/");
        while (token && component_count < 255) {
            if (strcmp(token, ".") == 0) {
                continue;
            } else if (strcmp(token, "..") == 0) {
                if (component_count > 0) {
                    component_count--;
                }
            } else {
                components[component_count++] = token;
            }
            token = strtok(NULL, "/");
        }
        
        normalized_path[0] = '\0';
        if (input_path[0] == '/') {
            strcat(normalized_path, "/");
        }
        
        for (int i = 0; i < component_count; i++) {
            if (i > 0 || input_path[0] == '/') {
                strcat(normalized_path, "/");
            }
            strcat(normalized_path, components[i]);
        }
        
        return true;
    }
    
    if (strlen(resolved) >= normalized_size) {
        free(resolved);
        return false;
    }
    
    strcpy(normalized_path, resolved);
    free(resolved);
    return true;
#endif
}

bool is_path_within_directory(const char *path, const char *base_dir) {
    if (!path || !base_dir) {
        return false;
    }

    char normalized_path[MAX_SAFE_PATH_LENGTH];
    char normalized_base[MAX_SAFE_PATH_LENGTH];

    if (!normalize_path_internal(path, normalized_path, sizeof(normalized_path)) ||
        !normalize_path_internal(base_dir, normalized_base, sizeof(normalized_base))) {
        return false;
    }

    size_t base_len = strlen(normalized_base);
    
    // Ensure base directory path ends with separator
    if (base_len > 0 && normalized_base[base_len - 1] != '/' && normalized_base[base_len - 1] != '\\') {
        if (base_len + 1 < sizeof(normalized_base)) {
#ifdef _WIN32
            strcat(normalized_base, "\\");
#else
            strcat(normalized_base, "/");
#endif
            base_len++;
        }
    }

    return (strncmp(normalized_path, normalized_base, base_len) == 0);
}

bool validate_safe_path(const char *path, char *safe_path, size_t safe_path_size) {
    if (!path || !safe_path || safe_path_size == 0) {
        return false;
    }

    if (strlen(path) >= MAX_SAFE_PATH_LENGTH) {
        return false;
    }

    // Check for null bytes
    if (strlen(path) != strcspn(path, "\0")) {
        return false;
    }

    // Normalize the path
    if (!normalize_path_internal(path, safe_path, safe_path_size)) {
        return false;
    }

    // Additional checks for dangerous patterns
    if (strstr(safe_path, "..") != NULL) {
        return false;
    }

    return true;
}

bool sanitize_argument(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return false;
    }

    size_t input_len = strlen(input);
    if (input_len == 0 || input_len >= output_size) {
        return false;
    }

    size_t output_pos = 0;
    
    for (size_t i = 0; i < input_len && output_pos < output_size - 1; i++) {
        char c = input[i];
        
        // Allow alphanumeric, basic punctuation, and path separators
        if (isalnum(c) || c == '.' || c == '-' || c == '_' || 
            c == '/' || c == '\\' || c == ':' || c == ' ') {
            output[output_pos++] = c;
        }
        // Skip dangerous characters
    }
    
    output[output_pos] = '\0';
    return true;
}
