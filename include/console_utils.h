#ifndef CONSOLE_UTILS_H
#define CONSOLE_UTILS_H

#include <stdbool.h>

typedef enum {
    COLOR_RESET = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BRIGHT_RED,
    COLOR_BRIGHT_GREEN,
    COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE,
    COLOR_BRIGHT_MAGENTA,
    COLOR_BRIGHT_CYAN,
    COLOR_BRIGHT_WHITE
} ConsoleColor;

void init_console_colors(void);
void print_colored(ConsoleColor color, const char *format, ...);
void print_error(const char *format, ...);
void print_success(const char *format, ...);
void print_warning(const char *format, ...);
void print_info(const char *format, ...);
void print_header(const char *text);
void print_progress(const char *text);

#endif
