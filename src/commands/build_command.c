#include "commands.h"
#include "compiler_utils.h"
#include "process_utils.h"
#include "toml_utils.h"
#include "include_utils.h"
#include "security_utils.h"
#include "crypto_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEFAULT_COMPILER_VERSION "v3.10.11"
#define DEFAULT_INPUT_FILE "gamemodes/main.pwn"
#define DEFAULT_OUTPUT_FILE "gamemodes/main.amx"
#define DEFAULT_TOML_FILE "opencli.toml"

// Error codes
#define ERR_SUCCESS 0
#define ERR_INVALID_INPUT 1
#define ERR_FILE_NOT_FOUND 2
#define ERR_COMPILATION_FAILED 3
#define ERR_SECURITY_VIOLATION 4

static bool copy_file(const char *source, const char *destination) {
    FILE *src_file, *dst_file;
    char buffer[4096];
    size_t bytes_read;

#ifdef _WIN32
    if (fopen_s(&src_file, source, "rb") != 0) {
        fprintf(stderr, "Failed to open source file: %s\n", source);
        return false;
    }

    if (fopen_s(&dst_file, destination, "wb") != 0) {
        fclose(src_file);
        fprintf(stderr, "Failed to open destination file: %s\n", destination);
        return false;
    }
#else
    src_file = fopen(source, "rb");
    if (src_file == NULL) {
        fprintf(stderr, "Failed to open source file: %s\n", source);
        return false;
    }

    dst_file = fopen(destination, "wb");
    if (dst_file == NULL) {
        fclose(src_file);
        fprintf(stderr, "Failed to open destination file: %s\n", destination);
        return false;
    }
#endif

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read) {
            fclose(src_file);
            fclose(dst_file);
            fprintf(stderr, "Error writing to destination file\n");
            return false;
        }
    }

    fclose(src_file);
    fclose(dst_file);
    return true;
}

static char* get_compiler_dll_path(const char *version) {
    static char dll_path[512];
    char version_without_v[32];
    
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
    
    const char *appdata_path = get_appdata_path();
    
#ifdef _WIN32
    sprintf_s(dll_path, sizeof(dll_path), "%s\\opencli\\compiler\\%s\\pawnc-%s-windows\\bin\\pawnc.dll", 
              appdata_path, version, version_without_v);
#else
    #ifdef __APPLE__
    sprintf(dll_path, "%s/opencli/compiler/%s/pawnc-%s-macos/lib/libpawnc.dylib", 
            appdata_path, version, version_without_v);
    #elif defined(__ANDROID__)
    sprintf(dll_path, "%s/opencli/compiler/%s/pawnc-%s-android/lib/libpawnc.so", 
            appdata_path, version, version_without_v);
    #else
    sprintf(dll_path, "%s/opencli/compiler/%s/pawnc-%s-linux/lib/libpawnc.so", 
            appdata_path, version, version_without_v);
    #endif
#endif

    return dll_path;
}

static void print_build_usage(void) {
    printf("Usage: opencli build [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --input <file>      Input file to compile (default: from opencli.toml or %s)\n", DEFAULT_INPUT_FILE);
    printf("  --output <file>     Output file (default: from opencli.toml or %s)\n", DEFAULT_OUTPUT_FILE);
    printf("  --compiler <ver>    Compiler version to use (default: from opencli.toml or %s)\n", DEFAULT_COMPILER_VERSION);
    printf("  --includes <dir>    Additional include directory\n");
    printf("  --help              Show this help message\n");
}

static bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * Process source file to detect missing includes (Enhanced with auto-append .inc)
 * 
 * @param source_file Source file path
 * @param include_dirs Array of include directories
 * @param include_dirs_count Number of include directories
 * @return true if all includes found, false if any missing
 */
static bool process_source_file_includes(const char *source_file, const char **include_dirs, int include_dirs_count) {
    FILE *file;
    char line[1024];
    bool all_includes_found = true;
    
    // Open source file
#ifdef _WIN32
    if (fopen_s(&file, source_file, "r") != 0) {
        fprintf(stderr, "Failed to open source file: %s\n", source_file);
        return false;
    }
#else
    file = fopen(source_file, "r");
    if (!file) {
        fprintf(stderr, "Failed to open source file: %s\n", source_file);
        return false;
    }
#endif
    
    // Get base directory from source file
    char *base_dir = get_directory_path(source_file);
    
    IncludeResolver *resolver = include_resolver_create(include_dirs, include_dirs_count, base_dir, true);
    if (!resolver) {
        fprintf(stderr, "Failed to create include resolver\n");
        fclose(file);
        return false;
    }
    
    // Read file line by line
    int line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        // Check if line contains #include
        if (strstr(line, "#include")) {
            IncludeInfo info;
            if (parse_include_statement(line, &info)) {
                // Use enhanced resolver to find include file
                char result_path[MAX_INCLUDE_PATH_LEN];
                if (!resolve_include_file(resolver, &info, result_path, sizeof(result_path))) {
                    fprintf(stderr, "Error: Cannot find include file '%s' at %s:%d\n", info.path, source_file, line_number);
                    all_includes_found = false;
                }
            }
        }
    }
    
    // Cleanup
    include_resolver_destroy(resolver);
    fclose(file);
    

    
    return all_includes_found;
}

static char* get_correct_input_path(const char *input_path) {
    static char correct_path[512];
    
    if (file_exists(input_path)) {
        #ifdef _WIN32
        strcpy_s(correct_path, sizeof(correct_path), input_path);
        #else
        strcpy(correct_path, input_path);
        #endif
        return correct_path;
    }
    
    const char *dot = strrchr(input_path, '.');
    if (dot != NULL) {
        #ifdef _WIN32
        strcpy_s(correct_path, sizeof(correct_path), input_path);
        #else
        strcpy(correct_path, input_path);
        #endif
        return correct_path;
    }
    
    #ifdef _WIN32
    sprintf_s(correct_path, sizeof(correct_path), "%s.pwn", input_path);
    #else
    sprintf(correct_path, "%s.pwn", input_path);
    #endif
    
    if (file_exists(correct_path)) {
        return correct_path;
    }
    
    #ifdef _WIN32
    strcpy_s(correct_path, sizeof(correct_path), input_path);
    #else
    strcpy(correct_path, input_path);
    #endif
    
    return correct_path;
}

int command_build(int argc, char *argv[]) {
    char input_file[512] = "";
    char output_file[512] = "";
    char compiler_version[32] = "";
    char includes[512] = "";
    char *compiler_path;
    bool have_includes = false;
    
    // Check if opencli.toml exists
    struct stat toml_st;
    bool has_toml = (stat(DEFAULT_TOML_FILE, &toml_st) == 0);
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_build_usage();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            char safe_path[MAX_SAFE_PATH_LENGTH];
            if (!validate_safe_path(argv[++i], safe_path, sizeof(safe_path))) {
                fprintf(stderr, "Security error: Invalid input path\n");
                return ERR_SECURITY_VIOLATION;
            }
            const char *allowed_input_exts[] = {"pwn", "pawn"};
            if (!validate_file_extension(safe_path, allowed_input_exts, 2)) {
                fprintf(stderr, "Error: Input file must be .pwn or .pawn\n");
                return ERR_INVALID_INPUT;
            }
            #ifdef _WIN32
            strcpy_s(input_file, sizeof(input_file), safe_path);
            #else
            strcpy(input_file, safe_path);
            #endif
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            char safe_path[MAX_SAFE_PATH_LENGTH];
            if (!validate_safe_path(argv[++i], safe_path, sizeof(safe_path))) {
                fprintf(stderr, "Security error: Invalid output path\n");
                return ERR_SECURITY_VIOLATION;
            }
            const char *allowed_output_exts[] = {"amx"};
            if (!validate_file_extension(safe_path, allowed_output_exts, 1)) {
                fprintf(stderr, "Error: Output file must be .amx\n");
                return ERR_INVALID_INPUT;
            }
            #ifdef _WIN32
            strcpy_s(output_file, sizeof(output_file), safe_path);
            #else
            strcpy(output_file, safe_path);
            #endif
        } else if (strcmp(argv[i], "--compiler") == 0 && i + 1 < argc) {
            char sanitized_version[32];
            if (!sanitize_argument(argv[++i], sanitized_version, sizeof(sanitized_version))) {
                fprintf(stderr, "Error: Invalid compiler version\n");
                return ERR_INVALID_INPUT;
            }
            #ifdef _WIN32
            strcpy_s(compiler_version, sizeof(compiler_version), sanitized_version);
            #else
            strcpy(compiler_version, sanitized_version);
            #endif
        } else if (strcmp(argv[i], "--includes") == 0 && i + 1 < argc) {
            char safe_path[MAX_SAFE_PATH_LENGTH];
            if (!validate_safe_path(argv[++i], safe_path, sizeof(safe_path))) {
                fprintf(stderr, "Security error: Invalid includes path\n");
                return ERR_SECURITY_VIOLATION;
            }
            #ifdef _WIN32
            strcpy_s(includes, sizeof(includes), safe_path);
            #else
            strcpy(includes, safe_path);
            #endif
            have_includes = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_build_usage();
            return EXIT_FAILURE;
        }
    }
    
    // Read values from opencli.toml if available
    if (has_toml) {
        // Only use toml values if command line options were not provided
        if (input_file[0] == '\0') {
            char *toml_input = read_toml_entry_file(DEFAULT_TOML_FILE);
            if (toml_input) {
                #ifdef _WIN32
                strcpy_s(input_file, sizeof(input_file), toml_input);
                #else
                strcpy(input_file, toml_input);
                #endif
            }
        }
        
        if (output_file[0] == '\0') {
            char *toml_output = read_toml_output_file(DEFAULT_TOML_FILE);
            if (toml_output) {
                #ifdef _WIN32
                strcpy_s(output_file, sizeof(output_file), toml_output);
                #else
                strcpy(output_file, toml_output);
                #endif
            }
        }
        
        if (compiler_version[0] == '\0') {
            char *toml_compiler = read_toml_compiler_version(DEFAULT_TOML_FILE);
            if (toml_compiler) {
                #ifdef _WIN32
                strcpy_s(compiler_version, sizeof(compiler_version), toml_compiler);
                #else
                strcpy(compiler_version, toml_compiler);
                #endif
            }
        }
        
        if (!have_includes) {
            int include_count = 0;
            char **include_paths = read_toml_include_paths(DEFAULT_TOML_FILE, &include_count);
            if (include_paths && include_count > 0) {
                have_includes = true;
            }
        }
    }
    
    // If still empty, use defaults
    if (input_file[0] == '\0') {
        #ifdef _WIN32
        strcpy_s(input_file, sizeof(input_file), DEFAULT_INPUT_FILE);
        #else
        strcpy(input_file, DEFAULT_INPUT_FILE);
        #endif
    }
    
    if (output_file[0] == '\0') {
        #ifdef _WIN32
        strcpy_s(output_file, sizeof(output_file), DEFAULT_OUTPUT_FILE);
        #else
        strcpy(output_file, DEFAULT_OUTPUT_FILE);
        #endif
    }
    
    if (compiler_version[0] == '\0') {
        #ifdef _WIN32
        strcpy_s(compiler_version, sizeof(compiler_version), DEFAULT_COMPILER_VERSION);
        #else
        strcpy(compiler_version, DEFAULT_COMPILER_VERSION);
        #endif
    }
    
    // Check if input file exists and use the correct extension
    char *correct_input_path = get_correct_input_path(input_file);
    if (strcmp(correct_input_path, input_file) != 0) {
        printf("Using input file: %s\n", correct_input_path);
        #ifdef _WIN32
        strcpy_s(input_file, sizeof(input_file), correct_input_path);
        #else
        strcpy(input_file, correct_input_path);
        #endif
    }
    
    // Now check if file exists with final path
    if (!file_exists(input_file)) {
        fprintf(stderr, "Input file not found: %s\n", input_file);
        fprintf(stderr, "Tried extensions: .pwn, .pawn\n");
        return EXIT_FAILURE;
    }
    
    // Check if compiler is installed, if not, install it
    if (!is_compiler_installed(compiler_version)) {
        printf("Compiler %s is not installed. Installing...\n", compiler_version);
        if (!install_compiler(compiler_version)) {
            fprintf(stderr, "Failed to install compiler %s\n", compiler_version);
            return EXIT_FAILURE;
        }
    }
    
    // Get compiler path
    compiler_path = get_compiler_path(compiler_version);
    if (!compiler_path) {
        fprintf(stderr, "Failed to get compiler path\n");
        return EXIT_FAILURE;
    }
    

    char output_path_without_ext[512];
    #ifdef _WIN32
    strcpy_s(output_path_without_ext, sizeof(output_path_without_ext), output_file);
    #else
    strcpy(output_path_without_ext, output_file);
    #endif

    // Remove .amx extension if present
    char *dot = strrchr(output_path_without_ext, '.');
    if (dot && strcmp(dot, ".amx") == 0) {
        *dot = '\0'; 
    }
    
#ifdef _WIN32
    char* dll_source_path = get_compiler_dll_path(compiler_version);
    char dll_dest_path[512] = "pawnc.dll"; 
    
    if (!copy_file(dll_source_path, dll_dest_path)) {
        fprintf(stderr, "Warning: Failed to copy pawnc.dll to current directory.\n");
        fprintf(stderr, "Compilation might fail if pawnc.dll is not in the PATH.\n");
    }
#endif
    
    
#ifdef _WIN32
    char batch_file[512];
    char working_dir[512];

    if (GetCurrentDirectory(sizeof(working_dir), working_dir) == 0) {
        strcpy_s(working_dir, sizeof(working_dir), ".");
    }
    
    
    sprintf_s(batch_file, sizeof(batch_file), "%s\\pawn_compile_temp.bat", working_dir);
    FILE* bat_file;
    if (fopen_s(&bat_file, batch_file, "w") != 0) {
        fprintf(stderr, "Failed to create temporary batch file for compilation\n");
        return EXIT_FAILURE;
    }
    
   
    fprintf(bat_file, "@echo off\n");
    fprintf(bat_file, "cd \"%s\"\n", working_dir);
    
    
    fprintf(bat_file, "\"%s\" ", compiler_path);
    
    
    if (has_toml) {
        int args_count = 0;
        char **compiler_args = read_toml_compiler_args(DEFAULT_TOML_FILE, &args_count);
        if (compiler_args && args_count > 0) {
            for (int i = 0; i < args_count; i++) {
                fprintf(bat_file, "%s ", compiler_args[i]);
            }
        } else {
            // Default compiler flags if no TOML args available
            fprintf(bat_file, "-d3 ");
            fprintf(bat_file, "-;+ ");
            fprintf(bat_file, "-(+ ");
            fprintf(bat_file, "-\\+ ");
            fprintf(bat_file, "-Z+ ");
            fprintf(bat_file, "-O1 "); 
        }
    } else {
        // Default compiler flags if no TOML file
        fprintf(bat_file, "-d3 ");
        fprintf(bat_file, "-;+ ");
        fprintf(bat_file, "-(+ ");
        fprintf(bat_file, "-\\+ ");
        fprintf(bat_file, "-Z+ ");
        fprintf(bat_file, "-O1 "); 
    }
    
    // Add input and output
    fprintf(bat_file, "\"%s\" ", input_file);
    fprintf(bat_file, "-o\"%s\" ", output_path_without_ext);
    
    // Add the directory of the input file as an include path for relative includes
    char *input_dir = get_directory_path(input_file);
    fprintf(bat_file, "-i\"%s\" ", input_dir);
    
    // Add include paths from command line
    if (have_includes && includes[0] != '\0') {
        fprintf(bat_file, "-i\"%s\" ", includes);
    }
    
    // Add includes from toml if available
    if (has_toml) {
        int include_count = 0;
        char **include_paths = read_toml_include_paths(DEFAULT_TOML_FILE, &include_count);
        if (include_paths && include_count > 0) {
            for (int i = 0; i < include_count; i++) {
                if (include_paths[i] && include_paths[i][0] != '\0') {
                    // Check if directory exists
                    struct stat dir_stat;
                    if (stat(include_paths[i], &dir_stat) == 0) {
                        fprintf(bat_file, "-i\"%s\" ", include_paths[i]);
                    }
                }
            }
        }
    }
    
    fclose(bat_file);
    
    
    const char *include_dirs[32]; 
    int include_dir_count = 0;
    
    
    include_dirs[include_dir_count++] = input_dir;
    
    
    if (have_includes && includes[0] != '\0') {
        include_dirs[include_dir_count++] = includes;
    }
    
    
    if (has_toml) {
        int toml_include_count = 0;
        char **toml_include_paths = read_toml_include_paths(DEFAULT_TOML_FILE, &toml_include_count);
        if (toml_include_paths && toml_include_count > 0) {
            for (int i = 0; i < toml_include_count && include_dir_count < 32; i++) {
                if (toml_include_paths[i] && toml_include_paths[i][0] != '\0') {
                    struct stat dir_stat;
                    if (stat(toml_include_paths[i], &dir_stat) == 0) {
                        include_dirs[include_dir_count++] = toml_include_paths[i];
                    }
                }
            }
        }
    }
    
    bool all_includes_found = process_source_file_includes(input_file, include_dirs, include_dir_count);
    
    if (!all_includes_found) {
        fprintf(stderr, "Error: Some include files could not be found. Compilation aborted.\n");
        fclose(bat_file);
        remove(batch_file);
        return EXIT_FAILURE;
    }
    
    int result = system(batch_file);
    
    // Delete temporary batch file
    remove(batch_file);
#else
    char *args[32]; 
    int arg_count = 0;
    
    args[arg_count++] = compiler_path;
    
    
    if (has_toml) {
        int args_count = 0;
        char **compiler_args = read_toml_compiler_args(DEFAULT_TOML_FILE, &args_count);
        if (compiler_args && args_count > 0) {
#ifdef __ANDROID__
            
            int total_len = 1; 
            for (int i = 0; i < args_count; i++) {
                total_len += strlen(compiler_args[i]) + 1; 
            }
            
            char *combined_flags = malloc(total_len); 
            if (combined_flags) {
                strcpy(combined_flags, "");  // Remove quotes - start empty
                for (int i = 0; i < args_count; i++) {
                    if (i > 0) strcat(combined_flags, " ");
                    strcat(combined_flags, compiler_args[i]);
                }
                // NO quotes added - exactly like manual execution
                args[arg_count++] = combined_flags;
            }
#else
            
            for (int i = 0; i < args_count && arg_count < 32; i++) {
                args[arg_count++] = compiler_args[i];
            }
#endif
        } else {
            
            args[arg_count++] = "-d3";
            args[arg_count++] = "-;+";
            args[arg_count++] = "-(+";
            args[arg_count++] = "-\\+";
            args[arg_count++] = "-Z+";
            args[arg_count++] = "-O1"; 
        }
    } else {
        
        args[arg_count++] = "-d3";
        args[arg_count++] = "-;+";
        args[arg_count++] = "-(+";
        args[arg_count++] = "-\\+";
        args[arg_count++] = "-Z+";
        args[arg_count++] = "-O1"; 
    }
    
    
    args[arg_count++] = input_file;
    
    
    char *output_arg = malloc(strlen(output_path_without_ext) + 3);
    if (output_arg) {
        sprintf(output_arg, "-o%s", output_path_without_ext);
        args[arg_count++] = output_arg;
    }
    

    
    
    if (have_includes && includes[0] != '\0') {
        char *include_arg = malloc(strlen(includes) + 3); 
        if (include_arg) {
            sprintf(include_arg, "-i%s", includes);
            args[arg_count++] = include_arg;
            printf("Adding include path from command line: %s\n", includes);
        }
    }
    
    
    if (has_toml) {
        int include_count = 0;
        char **include_paths = read_toml_include_paths(DEFAULT_TOML_FILE, &include_count);
        if (include_paths && include_count > 0) {
            for (int i = 0; i < include_count; i++) {
                if (include_paths[i] && include_paths[i][0] != '\0') {
                    struct stat dir_stat;
                    if (stat(include_paths[i], &dir_stat) == 0) {
                        char *include_arg = malloc(strlen(include_paths[i]) + 3);
                        if (include_arg) {
                            sprintf(include_arg, "-i%s", include_paths[i]);
                            args[arg_count++] = include_arg;
                            printf("Adding include path from TOML: %s\n", include_paths[i]);
                        }
                    } else {
                        printf("Warning: Include directory not found: %s (skipping)\n", include_paths[i]);
                    }
                }
            }
        }
    }
    
    args[arg_count] = NULL;
    
    
    const char *include_dirs[32]; 
    int include_dir_count = 0;
    
    
    if (have_includes && includes[0] != '\0') {
        include_dirs[include_dir_count++] = includes;
    }
    
    if (has_toml) {
        int toml_include_count = 0;
        char **toml_include_paths = read_toml_include_paths(DEFAULT_TOML_FILE, &toml_include_count);
        if (toml_include_paths && toml_include_count > 0) {
            for (int i = 0; i < toml_include_count && include_dir_count < 32; i++) {
                if (toml_include_paths[i] && toml_include_paths[i][0] != '\0') {
                    struct stat dir_stat;
                    if (stat(toml_include_paths[i], &dir_stat) == 0) {
                        include_dirs[include_dir_count++] = toml_include_paths[i];
                    }
                }
            }
        }
    }
    
    
    bool all_includes_found = process_source_file_includes(input_file, include_dirs, include_dir_count);
    
    if (!all_includes_found) {
        fprintf(stderr, "Error: Some include files could not be found. Compilation aborted.\n");
        
        
        for (int i = 11; i < arg_count; i++) {  
            free(args[i]);
        }
        
        return EXIT_FAILURE;
    }
    

    printf("Compiling %s to %s...\n", input_file, output_file);
    
    int result = run_process(compiler_path, args, true);
    
    for (int i = 11; i < arg_count; i++) { 
        free(args[i]);
    }
#endif
    
    
#ifdef _WIN32
    remove(dll_dest_path);
#endif
    
    // Comprehensive validation: Check both exit code AND output file existence
    bool exit_code_success = (result == 0);
    bool output_file_exists = false;
    
    // Check if output file was actually created
    struct stat output_stat;
    if (stat(output_file, &output_stat) == 0) {
        output_file_exists = true;
        
        // Additional validation: Check if file is not empty (valid .amx should have content)
        if (output_stat.st_size == 0) {
            output_file_exists = false;
        }
    }
    
    // Determine actual compilation success based on multiple factors
    bool compilation_successful = exit_code_success && output_file_exists;
    
    if (compilation_successful) {
        printf("Compilation successful!\n");
        printf("Output file: %s\n", output_file);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Compilation failed!\n");
        
        if (!exit_code_success) {
            fprintf(stderr, "  - Process exit code: %d\n", result);
        }
        
        if (!output_file_exists) {
            fprintf(stderr, "  - Output file not created or empty: %s\n", output_file);
        }
        
        // Additional debugging for Android/Termux FORTIFY issues
        #ifdef __ANDROID__
        fprintf(stderr, "\nAndroid/Termux troubleshooting:\n");
        fprintf(stderr, "  - If you see 'FORTIFY: fputs: null FILE*', this indicates a runtime issue\n");
        fprintf(stderr, "  - Try: export FORTIFY_SOURCE=0 before running OpenCLI\n");
        fprintf(stderr, "  - Ensure pawncc binary has execute permissions: chmod +x\n");
        #endif
        
        return EXIT_FAILURE;
    }
}