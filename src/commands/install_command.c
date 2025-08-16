#include "commands.h"
#include "compiler_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_COMPILER_VERSION "v3.10.11"

static void print_install_usage(void) {
    printf("Usage: opencli install <resource> [options]\n");
    printf("\n");
    printf("Resources:\n");
    printf("  compiler       Download and install Pawn compiler\n");
    printf("\n");
    printf("Options:\n");
    printf("  --version <ver>    Specify version to install (default: %s)\n", DEFAULT_COMPILER_VERSION);
    printf("  --help             Show this help message\n");
}

static int handle_install_compiler(int argc, char *argv[]) {
    char compiler_version[32] = DEFAULT_COMPILER_VERSION;
    bool version_specified = false;
    
    // Parse options
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_install_usage();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--version") == 0 && i + 1 < argc) {
            #ifdef _WIN32
            strcpy_s(compiler_version, sizeof(compiler_version), argv[++i]);
            #else
            strcpy(compiler_version, argv[++i]);
            #endif
            version_specified = true;
            
            // Ensure version starts with 'v'
            if (compiler_version[0] != 'v') {
                char temp[32];
                #ifdef _WIN32
                strcpy_s(temp, sizeof(temp), compiler_version);
                sprintf_s(compiler_version, sizeof(compiler_version), "v%s", temp);
                #else
                strcpy(temp, compiler_version);
                sprintf(compiler_version, "v%s", temp);
                #endif
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_install_usage();
            return EXIT_FAILURE;
        } else if (!version_specified) {
            // First positional argument is treated as version
            #ifdef _WIN32
            strcpy_s(compiler_version, sizeof(compiler_version), argv[i]);
            #else
            strcpy(compiler_version, argv[i]);
            #endif
            version_specified = true;
            
            // Ensure version starts with 'v'
            if (compiler_version[0] != 'v') {
                char temp[32];
                #ifdef _WIN32
                strcpy_s(temp, sizeof(temp), compiler_version);
                sprintf_s(compiler_version, sizeof(compiler_version), "v%s", temp);
                #else
                strcpy(temp, compiler_version);
                sprintf(compiler_version, "v%s", temp);
                #endif
            }
        }
    }
    
    printf("Installing Pawn compiler version %s\n", compiler_version);
    
    // Initialize compiler directory
    if (!init_compiler_dir()) {
        fprintf(stderr, "Failed to initialize compiler directory\n");
        return EXIT_FAILURE;
    }
    
    // Check if compiler is already installed
    if (is_compiler_installed(compiler_version)) {
        printf("Compiler version %s is already installed\n", compiler_version);
        return EXIT_SUCCESS;
    }
    
    // Install compiler
    if (!install_compiler(compiler_version)) {
        fprintf(stderr, "Failed to install compiler version %s\n", compiler_version);
        return EXIT_FAILURE;
    }
    
    printf("Compiler version %s installed successfully\n", compiler_version);
    return EXIT_SUCCESS;
}

int command_install(int argc, char *argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Missing resource to install\n");
        print_install_usage();
        return EXIT_FAILURE;
    }

    const char *resource = argv[0];

    if (strcmp(resource, "compiler") == 0) {
        return handle_install_compiler(argc - 1, &argv[1]);
    } else if (strcmp(resource, "--help") == 0 || strcmp(resource, "-h") == 0) {
        print_install_usage();
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Unknown resource: %s\n", resource);
        print_install_usage();
        return EXIT_FAILURE;
    }
} 