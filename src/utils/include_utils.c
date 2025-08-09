#include "include_utils.h"
#include "toml_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define getcwd _getcwd
#define access _access
#define F_OK 0
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#else
#include <unistd.h>
#include <limits.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

static const char* default_auto_extensions[] = {".inc"};
static const int default_auto_extensions_count = 1;

IncludeResolver* include_resolver_create(const char **include_dirs, int include_dirs_count, 
                                        const char *base_dir, bool enable_cache) {
    return include_resolver_create_advanced(include_dirs, include_dirs_count, base_dir, 
                                           enable_cache, true, default_auto_extensions, 
                                           default_auto_extensions_count);
}

IncludeResolver* include_resolver_create_advanced(const char **include_dirs, int include_dirs_count,
                                                 const char *base_dir, bool enable_cache,
                                                 bool auto_append_inc, const char **auto_extensions,
                                                 int auto_extensions_count) {
    IncludeResolver *resolver = calloc(1, sizeof(IncludeResolver));
    if (!resolver) return NULL;
    
    resolver->include_dirs = include_dirs;
    resolver->include_dirs_count = include_dirs_count;
    resolver->base_dir = base_dir;
    resolver->enable_cache = enable_cache;
    resolver->cache_count = 0;
    resolver->auto_append_inc = auto_append_inc;
    resolver->auto_extensions = auto_extensions;
    resolver->auto_extensions_count = auto_extensions_count;
    
    return resolver;
}

void include_resolver_destroy(IncludeResolver *resolver) {
    if (resolver) {
        free(resolver);
    }
}

void clear_include_cache(IncludeResolver *resolver) {
    if (resolver) {
        resolver->cache_count = 0;
        memset(resolver->cache, 0, sizeof(resolver->cache));
    }
}

void set_auto_append_inc(IncludeResolver *resolver, bool enabled) {
    if (resolver) {
        resolver->auto_append_inc = enabled;
    }
}

void set_auto_extensions(IncludeResolver *resolver, const char **extensions, int count) {
    if (resolver) {
        resolver->auto_extensions = extensions;
        resolver->auto_extensions_count = count;
    }
}

void normalize_path(char *path) {
    if (!path) return;
    
    char *p = path;
    while (*p) {
#ifdef _WIN32
        if (*p == '/') *p = '\\';
#else
        if (*p == '\\') *p = '/';
#endif
        p++;
    }
}

bool make_absolute_path(const char *relative_path, const char *base_dir, 
                       char *absolute_path, size_t path_size) {
    if (!relative_path || !absolute_path || path_size == 0) return false;
    
#ifdef _WIN32
    if (strlen(relative_path) >= 3 && relative_path[1] == ':') {
#else
    if (relative_path[0] == '/') {
#endif
        if (strlen(relative_path) >= path_size) return false;
        strcpy(absolute_path, relative_path);
        return true;
    }
    
    if (base_dir) {
        if (snprintf(absolute_path, path_size, "%s%c%s", 
                    base_dir, PATH_SEPARATOR, relative_path) >= (int)path_size) {
            return false;
        }
    } else {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) return false;
        
        if (snprintf(absolute_path, path_size, "%s%c%s", 
                    cwd, PATH_SEPARATOR, relative_path) >= (int)path_size) {
            return false;
        }
    }
    
    normalize_path(absolute_path);
    return true;
}

bool validate_include_path(const char *path) {
    if (!path) return false;
    
    if (strstr(path, "..") || strstr(path, "//") || strstr(path, "\\\\")) {
        return false;
    }
    
#ifdef _WIN32
    if (strlen(path) >= 3 && path[1] == ':') return false;
#else
    if (path[0] == '/') return false;
#endif
    
    return true;
}

bool parse_include_statement(const char *include_str, IncludeInfo *info) {
    if (!include_str || !info) return false;
    
    memset(info, 0, sizeof(IncludeInfo));
    
    const char *p = include_str;
    while (isspace(*p)) p++;
    
    if (*p != '#') return false;
    p++;
    
    while (isspace(*p)) p++;
    if (strncmp(p, "include", 7) != 0) return false;
    p += 7;
    
    while (isspace(*p)) p++;
    
    char quote_char = *p;
    if (quote_char == '<') {
        info->type = INCLUDE_TYPE_ANGLE;
        quote_char = '>';
    } else if (quote_char == '"') {
        info->type = INCLUDE_TYPE_QUOTE;
    } else {
        return false;
    }
    
    p++;
    const char *start = p;
    
    while (*p && *p != quote_char) p++;
    if (*p != quote_char) return false;
    
    size_t len = p - start;
    if (len == 0 || len >= MAX_INCLUDE_PATH_LEN) return false;
    
    strncpy(info->path, start, len);
    info->path[len] = '\0';
    
    if (info->type == INCLUDE_TYPE_QUOTE && !validate_include_path(info->path)) {
        return false;
    }
    
#ifdef _WIN32
    info->is_absolute = (len >= 3 && info->path[1] == ':');
#else
    info->is_absolute = (info->path[0] == '/');
#endif
    
    return true;
}



static IncludeCacheEntry* find_cache_entry(IncludeResolver *resolver, const char *include_path) {
    if (!resolver || !resolver->enable_cache || !include_path) return NULL;
    
    for (int i = 0; i < resolver->cache_count; i++) {
        if (strcmp(resolver->cache[i].include_path, include_path) == 0) {
            return &resolver->cache[i];
        }
    }
    return NULL;
}

static void add_cache_entry(IncludeResolver *resolver, const char *include_path, 
                           const char *resolved_path, bool exists) {
    if (!resolver || !resolver->enable_cache || !include_path) return;
    
    if (resolver->cache_count >= MAX_INCLUDE_CACHE_SIZE) {
        memmove(&resolver->cache[0], &resolver->cache[1], 
                sizeof(IncludeCacheEntry) * (MAX_INCLUDE_CACHE_SIZE - 1));
        resolver->cache_count = MAX_INCLUDE_CACHE_SIZE - 1;
    }
    
    IncludeCacheEntry *entry = &resolver->cache[resolver->cache_count++];
    strncpy(entry->include_path, include_path, MAX_INCLUDE_PATH_LEN - 1);
    entry->include_path[MAX_INCLUDE_PATH_LEN - 1] = '\0';
    
    if (resolved_path) {
        strncpy(entry->resolved_path, resolved_path, MAX_INCLUDE_PATH_LEN - 1);
        entry->resolved_path[MAX_INCLUDE_PATH_LEN - 1] = '\0';
    } else {
        entry->resolved_path[0] = '\0';
    }
    
    entry->exists = exists;
    entry->cached_time = time(NULL);
}

bool check_include_file_exists(const char *include_path) {
    if (!include_path || include_path[0] == '\0') return false;
    
#ifdef _WIN32
    return _access(include_path, F_OK) == 0;
#else
    return access(include_path, F_OK) == 0;
#endif
}

static bool try_resolve_path_with_extension(const char *base_path, const char *include_path, 
                                           const char *extension, char *result_path, 
                                           size_t result_size) {
    char test_path[MAX_INCLUDE_PATH_LEN];
    
    if (extension && strlen(extension) > 0) {
        if (snprintf(test_path, sizeof(test_path), "%s%c%s%s", 
                    base_path, PATH_SEPARATOR, include_path, extension) >= (int)sizeof(test_path)) {
            return false;
        }
    } else {
        if (snprintf(test_path, sizeof(test_path), "%s%c%s", 
                    base_path, PATH_SEPARATOR, include_path) >= (int)sizeof(test_path)) {
            return false;
        }
    }
    
    normalize_path(test_path);
    
    if (check_include_file_exists(test_path)) {
        if (result_size > strlen(test_path)) {
            strcpy(result_path, test_path);
            return true;
        }
    }
    
    return false;
}

static const char* valid_include_extensions[] = {".inc", ".pwn", ".p"};
static const int valid_include_extensions_count = 3;

static bool has_valid_include_extension(const char *include_path) {
    if (!include_path) return false;
    
    if (strcmp(include_path, "open.mp") == 0) {
        return false;
    }
    
    const char *last_dot = strrchr(include_path, '.');
    if (!last_dot) return false;
    
    const char *last_slash = strrchr(include_path, '/');
    const char *last_backslash = strrchr(include_path, '\\');
    const char *last_sep = NULL;
    
    if (last_slash && last_backslash) {
        last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
    } else if (last_slash) {
        last_sep = last_slash;
    } else if (last_backslash) {
        last_sep = last_backslash;
    }
    
    if (last_sep && last_dot < last_sep) {
        return false;
    }
    
    for (int i = 0; i < valid_include_extensions_count; i++) {
        if (strcmp(last_dot, valid_include_extensions[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

static bool try_resolve_path(IncludeResolver *resolver, const char *base_path, 
                            const char *include_path, char *result_path, size_t result_size) {
    if (!resolver || !base_path || !include_path || !result_path) return false;
    
    if (try_resolve_path_with_extension(base_path, include_path, NULL, 
                                       result_path, result_size)) {
        return true;
    }
    
    if (resolver->auto_append_inc) {
        if (!has_valid_include_extension(include_path)) {
            for (int i = 0; i < resolver->auto_extensions_count; i++) {
                if (resolver->auto_extensions[i]) {
                    if (try_resolve_path_with_extension(base_path, include_path, 
                                                       resolver->auto_extensions[i],
                                                       result_path, result_size)) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

bool resolve_include_file(IncludeResolver *resolver, const IncludeInfo *info,
                         char *result_path, size_t result_path_size) {
    if (!resolver || !info || !result_path || result_path_size == 0) return false;
    
    result_path[0] = '\0';
    
    if (resolver->enable_cache) {
        IncludeCacheEntry *cached = find_cache_entry(resolver, info->path);
        if (cached) {
            if (cached->exists && strlen(cached->resolved_path) < result_path_size) {
                strcpy(result_path, cached->resolved_path);
                return true;
            } else if (!cached->exists) {
                return false;
            }
        }
    }
    
    bool found = false;
    char resolved_path[MAX_INCLUDE_PATH_LEN];
    
    if (info->is_absolute) {
        if (check_include_file_exists(info->path)) {
            if (strlen(info->path) < result_path_size) {
                strcpy(result_path, info->path);
                strcpy(resolved_path, info->path);
                found = true;
            }
        }
    }
    else if (info->type == INCLUDE_TYPE_QUOTE) {
        if (resolver->base_dir) {
            if (try_resolve_path(resolver, resolver->base_dir, info->path, 
                               resolved_path, sizeof(resolved_path))) {
                if (strlen(resolved_path) < result_path_size) {
                    strcpy(result_path, resolved_path);
                    found = true;
                }
            }
        }
        
        if (!found) {
            for (int i = 0; i < resolver->include_dirs_count && !found; i++) {
                if (!resolver->include_dirs[i]) continue;
                
                if (try_resolve_path(resolver, resolver->include_dirs[i], info->path,
                                   resolved_path, sizeof(resolved_path))) {
                    if (strlen(resolved_path) < result_path_size) {
                        strcpy(result_path, resolved_path);
                        found = true;
                    }
                }
            }
        }
    }
    else if (info->type == INCLUDE_TYPE_ANGLE) {
        for (int i = 0; i < resolver->include_dirs_count && !found; i++) {
            if (!resolver->include_dirs[i]) continue;
            
            if (try_resolve_path(resolver, resolver->include_dirs[i], info->path,
                               resolved_path, sizeof(resolved_path))) {
                if (strlen(resolved_path) < result_path_size) {
                    strcpy(result_path, resolved_path);
                    found = true;
                }
            }
        }
    }
    
    if (resolver->enable_cache) {
        add_cache_entry(resolver, info->path, found ? resolved_path : NULL, found);
    }
    
    return found;
}

bool find_include_file(const IncludeInfo *info, const char *base_dir, 
                      const char **include_dirs, int include_dirs_count,
                      char *result_path, size_t result_path_size) {
    IncludeResolver *resolver = include_resolver_create(include_dirs, include_dirs_count, 
                                                       base_dir, false);
    if (!resolver) return false;
    
    bool result = resolve_include_file(resolver, info, result_path, result_path_size);
    include_resolver_destroy(resolver);
    
    return result;
}