#include "toml_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "toml.h"

#define DEFAULT_INPUT_FILE "gamemodes/main.pwn"
#define DEFAULT_OUTPUT_FILE "gamemodes/main.amx"
#define DEFAULT_COMPILER_VERSION "v3.10.11"

static void safe_strcpy(char* dest, size_t dest_size, const char* src) {
#ifdef _WIN32
    strcpy_s(dest, dest_size, src ? src : "");
#else
    if (src)
        strcpy(dest, src);
    else
        dest[0] = '\0';
#endif
}

static char* get_toml_string(toml_table_t* table, const char* key, const char* default_value) {
    static char result[256];
    
    safe_strcpy(result, sizeof(result), default_value);
    
    toml_datum_t datum = toml_string_in(table, key);
    if (datum.ok) {
        safe_strcpy(result, sizeof(result), datum.u.s);
        free(datum.u.s);
    }
    
    return result;
}

static toml_table_t* parse_toml_file(const char* toml_path) {
    FILE* file = fopen(toml_path, "r");
    if (!file) {
        return NULL;
    }
    
    char errbuf[200];
    toml_table_t* conf = toml_parse_file(file, errbuf, sizeof(errbuf));
    fclose(file);
    
    if (!conf) {
        fprintf(stderr, "Error parsing TOML file: %s\n", errbuf);
        return NULL;
    }
    
    return conf;
}

char* read_toml_entry_file(const char* toml_path) {
    toml_table_t* conf = parse_toml_file(toml_path);
    if (!conf) {
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_INPUT_FILE);
        return default_value;
    }
    
    toml_table_t* build = toml_table_in(conf, "build");
    if (!build) {
        toml_free(conf);
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_INPUT_FILE);
        return default_value;
    }
    
    char* result = get_toml_string(build, "entry_file", DEFAULT_INPUT_FILE);
    
    toml_free(conf);
    return result;
}

char* read_toml_output_file(const char* toml_path) {
    toml_table_t* conf = parse_toml_file(toml_path);
    if (!conf) {
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_OUTPUT_FILE);
        return default_value;
    }
    
    toml_table_t* build = toml_table_in(conf, "build");
    if (!build) {
        toml_free(conf);
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_OUTPUT_FILE);
        return default_value;
    }
    
    char* result = get_toml_string(build, "output_file", DEFAULT_OUTPUT_FILE);
    
    toml_free(conf);
    return result;
}

char* read_toml_compiler_version(const char* toml_path) {
    toml_table_t* conf = parse_toml_file(toml_path);
    if (!conf) {
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_COMPILER_VERSION);
        return default_value;
    }
    
    toml_table_t* build = toml_table_in(conf, "build");
    if (!build) {
        toml_free(conf);
        static char default_value[256];
        safe_strcpy(default_value, sizeof(default_value), DEFAULT_COMPILER_VERSION);
        return default_value;
    }
    
    char* result = get_toml_string(build, "compiler_version", DEFAULT_COMPILER_VERSION);
    
    toml_free(conf);
    return result;
}

char** read_toml_include_paths(const char* toml_path, int* count) {
    static char* paths[32]; // Support up to 32 include paths
    static char buffer[32][256]; // Each path can be up to 256 chars
    *count = 0;
    
    toml_table_t* conf = parse_toml_file(toml_path);
    if (!conf) {
        return NULL;
    }
    
    toml_table_t* build = toml_table_in(conf, "build");
    if (!build) {
        toml_free(conf);
        return NULL;
    }
    
    toml_table_t* includes = toml_table_in(build, "includes");
    if (!includes) {
        toml_free(conf);
        return NULL;
    }
    
    toml_array_t* paths_array = toml_array_in(includes, "paths");
    if (!paths_array) {
        toml_free(conf);
        return NULL;
    }
    
    // Get array size
    int i;
    for (i = 0; i < 32; i++) {
        toml_datum_t datum = toml_string_at(paths_array, i);
        if (!datum.ok) {
            break;
        }
        
        safe_strcpy(buffer[i], sizeof(buffer[i]), datum.u.s);
        paths[i] = buffer[i];
        free(datum.u.s);
    }
    
    *count = i;
    paths[*count] = NULL; // NULL terminate the array
    
    toml_free(conf);
    return paths;
}

char** read_toml_compiler_args(const char* toml_path, int* count) {
    static char* args[32]; // Support up to 32 compiler arguments
    static char buffer[32][256]; // Each argument can be up to 256 chars
    *count = 0;
    
    
    const char* default_args[] = {
        "-d3",
        "-;+",
        "-(+",
        "-\\+",
        "-Z+",
        "-O1",
        NULL
    };
    
    // Set default values first
    for (int i = 0; default_args[i] != NULL; i++) {
        safe_strcpy(buffer[i], sizeof(buffer[i]), default_args[i]);
        args[i] = buffer[i];
        (*count)++;
    }
    
    toml_table_t* conf = parse_toml_file(toml_path);
    if (!conf) {
        return args;
    }
    
    toml_table_t* build = toml_table_in(conf, "build");
    if (!build) {
        toml_free(conf);
        return args;
    }
    
    toml_table_t* build_args = toml_table_in(build, "args");
    if (!build_args) {
        toml_free(conf);
        return args;
    }
    
    toml_array_t* args_array = toml_array_in(build_args, "args");
    if (!args_array) {
        toml_free(conf);
        return args;
    }
    
    
    *count = 0;
    
    // Get array size
    for (int i = 0; i < 32; i++) {
        toml_datum_t datum = toml_string_at(args_array, i);
        if (!datum.ok) {
            break;
        }
        
        safe_strcpy(buffer[i], sizeof(buffer[i]), datum.u.s);
        args[i] = buffer[i];
        (*count)++;
        free(datum.u.s);
    }
    
    args[*count] = NULL;
    
    toml_free(conf);
    return args;
}

char* get_directory_path(const char* file_path) {
    static char dir_path[512];
    
    // Copy the file path to work with
    #ifdef _WIN32
    strcpy_s(dir_path, sizeof(dir_path), file_path);
    #else
    strcpy(dir_path, file_path);
    #endif
    
    // Find the last separator
    char* last_sep = NULL;
    
    #ifdef _WIN32
    // Windows: check for both '/' and '\'
    char* last_forward = strrchr(dir_path, '/');
    char* last_backward = strrchr(dir_path, '\\');
    
    if (last_forward && last_backward) {
        last_sep = (last_forward > last_backward) ? last_forward : last_backward;
    } else if (last_forward) {
        last_sep = last_forward;
    } else {
        last_sep = last_backward;
    }
    #else
    // Unix-like: only check for '/'
    last_sep = strrchr(dir_path, '/');
    #endif
    
    if (last_sep) {
        // Terminate the string at the separator
        *(last_sep + 1) = '\0';
    } else {
        // No directory part, use current directory
        #ifdef _WIN32
        strcpy_s(dir_path, sizeof(dir_path), ".\\");
        #else
        strcpy(dir_path, "./");
        #endif
    }
    
    return dir_path;
}

char* get_relative_include_path(const char* base_file, const char* include_file) {
    static char rel_path[512];
    char* base_dir = get_directory_path(base_file);
    
    #ifdef _WIN32
    sprintf_s(rel_path, sizeof(rel_path), "%s%s", base_dir, include_file);
    #else
    sprintf(rel_path, "%s%s", base_dir, include_file);
    #endif
    
    return rel_path;
}