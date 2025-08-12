#include "commands.h"
#include "download_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#define OPENCLI_TOML_URL "https://gist.githubusercontent.com/matyshh/62c80cf71a7f03f244218476ede47bf3/raw/opencli.toml"
#define OPENCLI_TOML_FILE "opencli.toml"

int command_setup(int argc, char *argv[]) {
    printf("OpenCLI Setup - Initializing open.mp project...\n");
    printf("================================================\n\n");
    
    // Check if opencli.toml already exists
    struct stat toml_stat;
    if (stat(OPENCLI_TOML_FILE, &toml_stat) == 0) {
        printf("Warning: opencli.toml already exists in current directory.\n");
        printf("Do you want to overwrite it? (y/N): ");
        
        char response[10];
        if (fgets(response, sizeof(response), stdin) != NULL) {
            if (response[0] != 'y' && response[0] != 'Y') {
                printf("Setup cancelled. Existing opencli.toml preserved.\n");
                return EXIT_SUCCESS;
            }
        }
        printf("\n");
    }
    
    printf("Downloading default opencli.toml configuration...\n");
    
    // Download the opencli.toml file
    bool download_result = download_file(OPENCLI_TOML_URL, OPENCLI_TOML_FILE);
    
    if (download_result) {
        printf("Successfully downloaded opencli.toml\n\n");
        
        printf("Setup Complete!\n");
        printf("===============\n\n");
        
        printf("Next steps:\n");
        printf("1. Edit opencli.toml to configure your project:\n");
        printf("   - Set compiler version and flags\n");
        printf("   - Configure include paths\n");
        printf("   - Adjust build settings\n\n");
        
        printf("2. Start using OpenCLI commands:\n");
        printf("   opencli build     - Build your Pawn scripts\n");
        printf("   opencli run       - Run your compiled scripts\n");
        printf("   opencli install   - Install compiler versions\n\n");
        
        printf("Configuration file: %s\n", OPENCLI_TOML_FILE);
        printf("For more help, check the documentation or run: opencli --help\n");
        
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Failed to download opencli.toml\n");
        fprintf(stderr, "Error code: %d\n\n", download_result);
        
        fprintf(stderr, "Troubleshooting:\n");
        fprintf(stderr, "- Check your internet connection\n");
        fprintf(stderr, "- Verify the URL is accessible: %s\n", OPENCLI_TOML_URL);
        fprintf(stderr, "- Try again later or create opencli.toml manually\n");
        
        return EXIT_FAILURE;
    }
}
