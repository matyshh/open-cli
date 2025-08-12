#include "compiler_utils.h"
#include "download_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <unistd.h>
#include <pwd.h>
#endif

#ifdef __ANDROID__
#define _GNU_SOURCE
#endif

#ifdef __ANDROID__
// Function to detect Android architecture using dpkg
static const char* detect_android_architecture() {
    FILE *fp;
    char output[32];
    static char arch[16] = {0};
    
    // Try dpkg --print-architecture first (Termux standard)
    fp = popen("dpkg --print-architecture 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(output, sizeof(output), fp) != NULL) {
            // Remove newline
            output[strcspn(output, "\n")] = 0;
            
            // Map dpkg architecture names to our naming convention
            if (strcmp(output, "aarch64") == 0 || strcmp(output, "arm64") == 0) {
                strcpy(arch, "arm64");
                pclose(fp);
                return arch;
            } else if (strcmp(output, "armhf") == 0 || strcmp(output, "armeabi-v7a") == 0) {
                strcpy(arch, "arm32");
                pclose(fp);
                return arch;
            }
        }
        pclose(fp);
    }
    
    // Fallback: try uname -m
    fp = popen("uname -m 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(output, sizeof(output), fp) != NULL) {
            output[strcspn(output, "\n")] = 0;
            
            if (strstr(output, "aarch64") || strstr(output, "arm64")) {
                strcpy(arch, "arm64");
                pclose(fp);
                return arch;
            } else if (strstr(output, "arm")) {
                strcpy(arch, "arm32");
                pclose(fp);
                return arch;
            }
        }
        pclose(fp);
    }
    
    // Default fallback to arm64 (most common in modern Termux)
    strcpy(arch, "arm64");
    return arch;
}
#endif

#define COMPILERS_TOML_URL "https://gist.githubusercontent.com/weltschmerzie/03dce551fec8d20a25b99545e652ae5f/raw/compilers.toml"

static char compiler_base_dir[512] = {0};
static char opencli_dir[512] = {0};
static FILE* log_file = NULL;
static int verbose_logging = 0;

static bool create_directory(const char *path);
static bool ensure_directory_exists(const char *path);

void set_compiler_verbose_logging(bool verbose) {
    verbose_logging = verbose ? 1 : 0;
}

const char *get_appdata_path(void) {
    static char path[512] = {0};
    
    if (path[0] != '\0') {
        return path;
    }

#ifdef _WIN32
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return path;
    }
#else
    const char *home_dir = NULL;
    
    home_dir = getenv("HOME");
    
    if (!home_dir) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home_dir = pw->pw_dir;
        }
    }
    
    if (home_dir) {
        #ifdef _WIN32
        strcpy_s(path, sizeof(path), home_dir);
        strcat_s(path, sizeof(path), "/.config");
        #else
        strcpy(path, home_dir);
        strcat(path, "/.config");
        #endif
        return path;
    }
#endif

    #ifdef _WIN32
    strcpy_s(path, sizeof(path), ".");
    #else
    strcpy(path, ".");
    #endif
    return path;
}

static void log_message(const char* format, ...) {
    va_list args;
    char buffer[4096];
    time_t now;
    struct tm timeinfo_obj;
    struct tm* timeinfo;
    char timestamp[26];
    
    time(&now);
    
    #ifdef _WIN32
    localtime_s(&timeinfo_obj, &now);
    timeinfo = &timeinfo_obj;
    #else
    timeinfo = localtime(&now);
    #endif
    
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (verbose_logging) {
        printf("[%s] %s\n", timestamp, buffer);
        fflush(stdout);
    }
    
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", timestamp, buffer);
        fflush(log_file);
    }
}

static void init_log_file() {
    char log_path[512];
    
    if (log_file) {
        return;
    }
    
    const char *appdata_path = get_appdata_path();
    
    #ifdef _WIN32
    sprintf_s(log_path, sizeof(log_path), "%s\\opencli\\opencli.log", appdata_path);
    #else
    sprintf(log_path, "%s/opencli/opencli.log", appdata_path);
    #endif
    
    #ifdef _WIN32
    fopen_s(&log_file, log_path, "a");
    #else
    log_file = fopen(log_path, "a");
    #endif
    
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_path);
    }
}

static void close_log_file() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

static bool create_directory(const char *path) {
    struct stat st = {0};
    
    if (stat(path, &st) == 0) {
        return true;
    }
    
#ifdef _WIN32
    if (_mkdir(path) == 0) {
        log_message("Created directory: %s", path);
        return true;
    } else {
        log_message("Failed to create directory: %s, error: %d", path, errno);
        return false;
    }
#else
    if (mkdir(path, 0755) == 0) {
        log_message("Created directory: %s", path);
        return true;
    } else {
        log_message("Failed to create directory: %s, error: %d", path, errno);
        return false;
    }
#endif
}

static bool ensure_directory_exists(const char *path) {
    char tmp[512];
    char *p;
    
    log_message("Ensuring directory exists: %s", path);
    
    #ifdef _WIN32
    strcpy_s(tmp, sizeof(tmp), path);
    
    // Skip drive letter on Windows (e.g., "C:")
    p = tmp;
    if (strlen(tmp) >= 2 && tmp[1] == ':') {
        p = tmp + 2;  
        if (*p == '\\' || *p == '/') {
            p++;  
        }
    }
    #else
    strcpy(tmp, path);
    p = tmp;
    #endif
    
    // Iterate through path and create each directory
    for (; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            if (strlen(tmp) > 0 && !create_directory(tmp)) {
                return false;
            }
            *p = '/'; 
        }
    }
    
    return create_directory(tmp);
}

// Try an alternative download method
static bool try_alternative_download(const char *url, const char *dest_path) {
    char cmd[4096];
    log_message("Trying alternative download method...");
    log_message("URL: %s", url);
    log_message("Destination: %s", dest_path);

#ifdef _WIN32
    // Use PowerShell to download
    sprintf_s(cmd, sizeof(cmd), "powershell -Command \"& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing}\"", url, dest_path);
    log_message("Executing command: %s", cmd);
    int result = system(cmd);
    if (result == 0) {
        log_message("PowerShell download successful");
        return true;
    } else {
        log_message("PowerShell download failed with exit code: %d", result);
        return false;
    }
#else
    // Use curl or wget
    if (system("which curl > /dev/null 2>&1") == 0) {
        sprintf(cmd, "curl -L '%s' -o '%s'", url, dest_path);
    } else if (system("which wget > /dev/null 2>&1") == 0) {
        sprintf(cmd, "wget '%s' -O '%s'", url, dest_path);
    } else {
        log_message("Neither curl nor wget found");
        return false;
    }
    
    log_message("Executing command: %s", cmd);
    int result = system(cmd);
    if (result == 0) {
        log_message("Command-line download successful");
        return true;
    } else {
        log_message("Command-line download failed with exit code: %d", result);
        return false;
    }
#endif
}

bool init_compiler_dir(void) {
    const char *appdata_path = get_appdata_path();
    
    init_log_file();
    log_message("Initializing compiler directory...");
    
#ifdef _WIN32
    sprintf_s(opencli_dir, sizeof(opencli_dir), "%s\\opencli", appdata_path);
    sprintf_s(compiler_base_dir, sizeof(compiler_base_dir), "%s\\opencli\\compiler", appdata_path);
#else
    sprintf(opencli_dir, "%s/opencli", appdata_path);
    sprintf(compiler_base_dir, "%s/opencli/compiler", appdata_path);
#endif
    
    log_message("AppData path: %s", appdata_path);
    log_message("OpenCLI directory: %s", opencli_dir);
    log_message("Compiler base directory: %s", compiler_base_dir);
    
    if (!ensure_directory_exists(opencli_dir)) {
        log_message("Failed to create opencli directory");
        return false;
    }
    
    if (!ensure_directory_exists(compiler_base_dir)) {
        log_message("Failed to create compiler directory");
        return false;
    }
    
    char compilers_toml_path[512];
#ifdef _WIN32
    sprintf_s(compilers_toml_path, sizeof(compilers_toml_path), "%s\\compilers.toml", opencli_dir);
#else
    sprintf(compilers_toml_path, "%s/compilers.toml", opencli_dir);
#endif

    struct stat st = {0};
    if (stat(compilers_toml_path, &st) != 0) {
        log_message("Downloading compilers.toml from GitHub...");
        if (!download_file(COMPILERS_TOML_URL, compilers_toml_path)) {
            log_message("Failed to download compilers.toml, trying alternative method");
            if (!try_alternative_download(COMPILERS_TOML_URL, compilers_toml_path)) {
                log_message("Failed to download compilers.toml with alternative method");
                return false;
            }
        }
    }
    
    log_message("Compiler directory initialized successfully");
    return true;
}

bool is_compiler_installed(const char *version) {
    char path_exe[512];
    char path_dll[512];
    struct stat st_exe = {0};
    struct stat st_dll = {0};
    char version_without_v[32];
    
    if (!init_compiler_dir()) {
        log_message("Failed to initialize compiler directory when checking installation");
        return false;
    }
    
    if (version[0] == 'v') {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version + 1);
        #else
        strcpy(version_without_v, version + 1);
        #endif
    } else {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version);
        #else
        strcpy(version_without_v, version);
        #endif
    }
    
#ifdef _WIN32
    sprintf_s(path_exe, sizeof(path_exe), "%s\\%s\\pawnc-%s-windows\\bin\\pawncc.exe", compiler_base_dir, version, version_without_v);
    sprintf_s(path_dll, sizeof(path_dll), "%s\\%s\\pawnc-%s-windows\\bin\\pawnc.dll", compiler_base_dir, version, version_without_v);
#else
    #ifdef __APPLE__
    sprintf(path_exe, "%s/%s/pawnc-%s-macos/bin/pawncc", compiler_base_dir, version, version_without_v);
    sprintf(path_dll, "%s/%s/pawnc-%s-macos/lib/libpawnc.dylib", compiler_base_dir, version, version_without_v);
    #elif defined(__ANDROID__)
    const char* android_arch = detect_android_architecture();
    sprintf(path_exe, "%s/%s-%s/bin/pawncc", compiler_base_dir, version, android_arch);
    sprintf(path_dll, "%s/%s-%s/lib/libpawnc.so", compiler_base_dir, version, android_arch);
    #else
    sprintf(path_exe, "%s/%s/pawnc-%s-linux/bin/pawncc", compiler_base_dir, version, version_without_v);
    sprintf(path_dll, "%s/%s/pawnc-%s-linux/lib/libpawnc.so", compiler_base_dir, version, version_without_v);
    #endif
#endif
    
    log_message("Checking if compiler executable exists: %s", path_exe);
    log_message("Checking if compiler library exists: %s", path_dll);
    
    // Check if both compiler binary and library exist
    bool exe_exists = (stat(path_exe, &st_exe) == 0);
    bool dll_exists = (stat(path_dll, &st_dll) == 0);
    
    bool installed = exe_exists && dll_exists;
    
    log_message("Compiler %s executable exists: %s", version, exe_exists ? "true" : "false");
    log_message("Compiler %s library exists: %s", version, dll_exists ? "true" : "false");
    log_message("Compiler %s is %s", version, installed ? "installed" : "not installed");
    
    return installed;
}

char *get_compiler_path(const char *version) {
    static char path[512];
    char version_without_v[32];
    
    // Init compiler directory
    if (!init_compiler_dir()) {
        log_message("Failed to initialize compiler directory when getting path");
        return NULL;
    }
    
    // Remove 'v' prefix if present for directory name
    if (version[0] == 'v') {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version + 1);
        #else
        strcpy(version_without_v, version + 1);
        #endif
    } else {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version);
        #else
        strcpy(version_without_v, version);
        #endif
    }
    
    // Build path to compiler binary
#ifdef _WIN32
    sprintf_s(path, sizeof(path), "%s\\%s\\pawnc-%s-windows\\bin\\pawncc.exe", compiler_base_dir, version, version_without_v);
#else
    #ifdef __APPLE__
    sprintf(path, "%s/%s/pawnc-%s-macos/bin/pawncc", compiler_base_dir, version, version_without_v);
    #elif defined(__ANDROID__)
    const char* android_arch = detect_android_architecture();
    sprintf(path, "%s/%s-%s/bin/pawncc", compiler_base_dir, version, android_arch);
    #else
    sprintf(path, "%s/%s/pawnc-%s-linux/bin/pawncc", compiler_base_dir, version, version_without_v);
    #endif
#endif
    
    log_message("Compiler path: %s", path);
    return path;
}

bool install_compiler(const char *version) {
    char url[512];
    char zip_path[512];
    char extract_dir[512];
    char version_without_v[32];
#ifdef __ANDROID__
    const char* android_arch = NULL;
#endif
    
    log_message("Installing compiler version: %s", version);
    
    if (!init_compiler_dir()) {
        log_message("Failed to initialize compiler directory when installing");
        return false;
    }
    
    if (version[0] == 'v') {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version + 1);
        #else
        strcpy(version_without_v, version + 1);
        #endif
    } else {
        #ifdef _WIN32
        strcpy_s(version_without_v, sizeof(version_without_v), version);
        #else
        strcpy(version_without_v, version);
        #endif
    }
    

    const char *repo_url;
    
#ifdef __ANDROID__
    repo_url = "https://github.com/matyshh/compiler";
#else
    int major, minor, patch;
    const char *version_parse = (version[0] == 'v') ? version + 1 : version;
    
    if (sscanf(version_parse, "%d.%d.%d", &major, &minor, &patch) == 3) {
        // Compare semantic version: 3.10.11+ uses openmultiplayer
        if (major > 3 || (major == 3 && (minor > 10 || (minor == 10 && patch >= 11)))) {
            repo_url = "https://github.com/openmultiplayer/compiler";
        } else {
            repo_url = "https://github.com/pawn-lang/compiler";
        }
    } else {
        // Fallback for unparseable versions - use pawn-lang
        repo_url = "https://github.com/pawn-lang/compiler";
    }
#endif
    
#ifdef _WIN32
    sprintf_s(url, sizeof(url), "%s/releases/download/%s/pawnc-%s-windows.zip", repo_url, version, version_without_v);
    sprintf_s(zip_path, sizeof(zip_path), "%s\\%s.zip", compiler_base_dir, version);
    sprintf_s(extract_dir, sizeof(extract_dir), "%s\\%s", compiler_base_dir, version);
#else
    #ifdef __APPLE__
    sprintf(url, "%s/releases/download/%s/pawnc-%s-macos.zip", repo_url, version, version_without_v);
    sprintf(zip_path, "%s/%s.zip", compiler_base_dir, version);
    sprintf(extract_dir, "%s/%s", compiler_base_dir, version);
    #elif defined(__ANDROID__)
    android_arch = detect_android_architecture();
    sprintf(url, "%s/releases/download/%s/pawnc-%s-android-%s.zip", repo_url, version_without_v, version_without_v, android_arch);
    sprintf(zip_path, "%s/%s-%s.zip", compiler_base_dir, version, android_arch);
    sprintf(extract_dir, "%s/%s-%s", compiler_base_dir, version, android_arch);
    #else
    sprintf(url, "%s/releases/download/%s/pawnc-%s-linux.tar.gz", repo_url, version, version_without_v);
    sprintf(zip_path, "%s/%s.tar.gz", compiler_base_dir, version);
    sprintf(extract_dir, "%s/%s", compiler_base_dir, version);
    #endif
#endif

    log_message("Download URL: %s", url);
    log_message("Zip path: %s", zip_path);
    log_message("Extract dir: %s", extract_dir);

    if (!ensure_directory_exists(extract_dir)) {
        log_message("Failed to create extraction directory");
        return false;
    }

    log_message("Downloading compiler %s...", version);
    if (!download_file(url, zip_path)) {
        log_message("Failed to download compiler, trying alternative method");
        if (!try_alternative_download(url, zip_path)) {
            log_message("Failed to download compiler with alternative method");
            return false;
        }
    }
    
    struct stat st = {0};
    if (stat(zip_path, &st) != 0) {
        log_message("Downloaded file not found: %s", zip_path);
        return false;
    }
    
    log_message("Download successful. File size: %lld bytes", (long long)st.st_size);
    
    log_message("Extracting compiler to %s...", extract_dir);
#ifdef _WIN32
    if (!extract_zip(zip_path, extract_dir)) {
        log_message("Failed to extract compiler using builtin method, trying PowerShell");
        char cmd[4096];
        sprintf_s(cmd, sizeof(cmd), "powershell -Command \"& {Expand-Archive -Path '%s' -DestinationPath '%s' -Force}\"", zip_path, extract_dir);
        log_message("Executing command: %s", cmd);
        int result = system(cmd);
        if (result != 0) {
            log_message("PowerShell extraction failed with exit code: %d", result);
            return false;
        }
    }
#else
    #ifdef __APPLE__
    if (!extract_zip(zip_path, extract_dir)) {
        log_message("Failed to extract compiler");
        return false;
    }
    #elif defined(__ANDROID__)
    if (!extract_zip(zip_path, extract_dir)) {
        log_message("Failed to extract compiler");
        return false;
    }
    #else
    if (!extract_tgz(zip_path, extract_dir)) {
        log_message("Failed to extract compiler");
        return false;
    }
    #endif
#endif
    
    char pawncc_path[512];
    char pawnc_path[512];

#ifdef _WIN32
    sprintf_s(pawncc_path, sizeof(pawncc_path), "%s\\pawnc-%s-windows\\bin\\pawncc.exe", extract_dir, version_without_v);
    sprintf_s(pawnc_path, sizeof(pawnc_path), "%s\\pawnc-%s-windows\\bin\\pawnc.dll", extract_dir, version_without_v);
#else
    #ifdef __APPLE__
    sprintf(pawncc_path, "%s/pawnc-%s-macos/bin/pawncc", extract_dir, version_without_v);
    sprintf(pawnc_path, "%s/pawnc-%s-macos/lib/libpawnc.dylib", extract_dir, version_without_v);
    #elif defined(__ANDROID__)
    sprintf(pawncc_path, "%s/bin/pawncc", extract_dir);
    sprintf(pawnc_path, "%s/lib/libpawnc.so", extract_dir);
    #else
    sprintf(pawncc_path, "%s/pawnc-%s-linux/bin/pawncc", extract_dir, version_without_v);
    sprintf(pawnc_path, "%s/pawnc-%s-linux/lib/libpawnc.so", extract_dir, version_without_v);
    #endif
#endif

    struct stat st_exe = {0};
    struct stat st_dll = {0};
    bool exe_exists = (stat(pawncc_path, &st_exe) == 0);
    bool dll_exists = (stat(pawnc_path, &st_dll) == 0);

    if (!exe_exists || !dll_exists) {
        log_message("Required compiler files not found after extraction:");
        log_message("  Executable (%s): %s", pawncc_path, exe_exists ? "Found" : "Missing");
        log_message("  Library (%s): %s", pawnc_path, dll_exists ? "Found" : "Missing");
        
        return false;
    }
    
#ifdef __ANDROID__
    if (chmod(pawncc_path, 0755) != 0) {
        log_message("Warning: Failed to make pawncc executable: %s", strerror(errno));
    } else {
        log_message("Made pawncc executable with chmod +x");
    }
#endif
    
    log_message("Compiler %s installed successfully", version);
    log_message("Executable path: %s", pawncc_path);
    log_message("Library path: %s", pawnc_path);
    return true;
} 