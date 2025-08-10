#ifndef OPENCLI_INCLUDE_UTILS_H
#define OPENCLI_INCLUDE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define MAX_INCLUDE_PATH_LEN 1024
#define MAX_INCLUDE_CACHE_SIZE 256
#define MAX_INCLUDE_DEPTH 32

typedef enum {
    INCLUDE_TYPE_ANGLE,
    INCLUDE_TYPE_QUOTE
} IncludeType;

typedef struct {
    char path[MAX_INCLUDE_PATH_LEN];
    IncludeType type;
    bool is_absolute;
    char resolved_path[MAX_INCLUDE_PATH_LEN];
} IncludeInfo;

typedef struct {
    char include_path[MAX_INCLUDE_PATH_LEN];
    char resolved_path[MAX_INCLUDE_PATH_LEN];
    bool exists;
    time_t cached_time;
} IncludeCacheEntry;

typedef struct {
    const char **include_dirs;
    int include_dirs_count;
    const char *base_dir;
    IncludeCacheEntry cache[MAX_INCLUDE_CACHE_SIZE];
    int cache_count;
    bool enable_cache;
    bool auto_append_inc;
    const char **auto_extensions;
    int auto_extensions_count;
} IncludeResolver;

IncludeResolver* include_resolver_create(const char **include_dirs, int include_dirs_count, 
                                        const char *base_dir, bool enable_cache);

IncludeResolver* include_resolver_create_advanced(const char **include_dirs, int include_dirs_count,
                                                 const char *base_dir, bool enable_cache,
                                                 bool auto_append_inc, const char **auto_extensions,
                                                 int auto_extensions_count);

void include_resolver_destroy(IncludeResolver *resolver);

bool parse_include_statement(const char *include_str, IncludeInfo *info);

bool resolve_include_file(IncludeResolver *resolver, const IncludeInfo *info,
                         char *result_path, size_t result_path_size);

// Legacy compatibility
bool find_include_file(const IncludeInfo *info, const char *base_dir, 
                      const char **include_dirs, int include_dirs_count,
                      char *result_path, size_t result_path_size);

bool check_include_file_exists(const char *include_path);
void normalize_path(char *path);
bool make_absolute_path(const char *relative_path, const char *base_dir, 
                       char *absolute_path, size_t path_size);
bool validate_include_path(const char *path);
void clear_include_cache(IncludeResolver *resolver);
void set_auto_append_inc(IncludeResolver *resolver, bool enabled);
void set_auto_extensions(IncludeResolver *resolver, const char **extensions, int count);

#endif /* OPENCLI_INCLUDE_UTILS_H */