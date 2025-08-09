#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "compiler_utils.h"
#include "console_utils.h"

void print_usage(void) {
    print_colored(COLOR_BRIGHT_WHITE, "Usage: ");
    print_colored(COLOR_CYAN, "opencli ");
    print_colored(COLOR_YELLOW, "<command> ");
    print_colored(COLOR_WHITE, "[options]\n\n");
    
    print_colored(COLOR_BRIGHT_BLUE, "Commands:\n");
    print_colored(COLOR_GREEN, "  run         ");
    printf("Start an open.mp server\n");
    print_colored(COLOR_GREEN, "  build       ");
    printf("Compile Pawn scripts\n");
    print_colored(COLOR_GREEN, "  install     ");
    printf("Install resources (compiler, etc.)\n");
    printf("\n");
    print_info("For more information: ");
    print_colored(COLOR_CYAN, "opencli <command> --help\n");
}

int main(int argc, char *argv[]) {
    init_console_colors();
    set_compiler_verbose_logging(false);
    
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    const char *command = argv[1];

    if (strcmp(command, "run") == 0) {
        return command_run(argc - 2, &argv[2]);
    } else if (strcmp(command, "build") == 0) {
        return command_build(argc - 2, &argv[2]);
    } else if (strcmp(command, "install") == 0) {
        return command_install(argc - 2, &argv[2]);
    } else if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage();
        return EXIT_SUCCESS;
    } else {
        print_error("Unknown command: %s\n", command);
        printf("Run 'opencli --help' for available commands.\n");
        return EXIT_FAILURE;
    }
}