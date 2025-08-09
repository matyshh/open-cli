#include "console_utils.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

static bool colors_enabled = false;

#ifdef _WIN32
static HANDLE hConsole = INVALID_HANDLE_VALUE;
static WORD original_attributes = 0;

static WORD get_win_color(ConsoleColor color) {
    switch (color) {
        case COLOR_RED: return FOREGROUND_RED;
        case COLOR_GREEN: return FOREGROUND_GREEN;
        case COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
        case COLOR_BLUE: return FOREGROUND_BLUE;
        case COLOR_MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE;
        case COLOR_CYAN: return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case COLOR_WHITE: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case COLOR_BRIGHT_RED: return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_GREEN: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_BLUE: return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_CYAN: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case COLOR_BRIGHT_WHITE: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        default: return original_attributes;
    }
}
#else
static const char* get_ansi_color(ConsoleColor color) {
    switch (color) {
        case COLOR_RESET: return "\033[0m";
        case COLOR_RED: return "\033[31m";
        case COLOR_GREEN: return "\033[32m";
        case COLOR_YELLOW: return "\033[33m";
        case COLOR_BLUE: return "\033[34m";
        case COLOR_MAGENTA: return "\033[35m";
        case COLOR_CYAN: return "\033[36m";
        case COLOR_WHITE: return "\033[37m";
        case COLOR_BRIGHT_RED: return "\033[91m";
        case COLOR_BRIGHT_GREEN: return "\033[92m";
        case COLOR_BRIGHT_YELLOW: return "\033[93m";
        case COLOR_BRIGHT_BLUE: return "\033[94m";
        case COLOR_BRIGHT_MAGENTA: return "\033[95m";
        case COLOR_BRIGHT_CYAN: return "\033[96m";
        case COLOR_BRIGHT_WHITE: return "\033[97m";
        default: return "\033[0m";
    }
}
#endif

void init_console_colors(void) {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
            original_attributes = consoleInfo.wAttributes;
            colors_enabled = true;
        }
    }
#else
    colors_enabled = isatty(fileno(stdout));
#endif
}

void print_colored(ConsoleColor color, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    if (colors_enabled && color != COLOR_RESET) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, get_win_color(color));
        }
#else
        printf("%s", get_ansi_color(color));
#endif
    }
    
    vprintf(format, args);
    
    if (colors_enabled && color != COLOR_RESET) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, original_attributes);
        }
#else
        printf("%s", get_ansi_color(COLOR_RESET));
#endif
    }
    
    va_end(args);
}

void print_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    print_colored(COLOR_BRIGHT_RED, "[ERROR] ");
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, get_win_color(COLOR_RED));
        }
#else
        printf("%s", get_ansi_color(COLOR_RED));
#endif
    }
    
    vprintf(format, args);
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, original_attributes);
        }
#else
        printf("%s", get_ansi_color(COLOR_RESET));
#endif
    }
    
    va_end(args);
}

void print_success(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    print_colored(COLOR_BRIGHT_GREEN, "[OK] ");
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, get_win_color(COLOR_GREEN));
        }
#else
        printf("%s", get_ansi_color(COLOR_GREEN));
#endif
    }
    
    vprintf(format, args);
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, original_attributes);
        }
#else
        printf("%s", get_ansi_color(COLOR_RESET));
#endif
    }
    
    va_end(args);
}

void print_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    print_colored(COLOR_BRIGHT_YELLOW, "[WARN] ");
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, get_win_color(COLOR_YELLOW));
        }
#else
        printf("%s", get_ansi_color(COLOR_YELLOW));
#endif
    }
    
    vprintf(format, args);
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, original_attributes);
        }
#else
        printf("%s", get_ansi_color(COLOR_RESET));
#endif
    }
    
    va_end(args);
}

void print_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    print_colored(COLOR_BRIGHT_CYAN, "[INFO] ");
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, get_win_color(COLOR_CYAN));
        }
#else
        printf("%s", get_ansi_color(COLOR_CYAN));
#endif
    }
    
    vprintf(format, args);
    
    if (colors_enabled) {
#ifdef _WIN32
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, original_attributes);
        }
#else
        printf("%s", get_ansi_color(COLOR_RESET));
#endif
    }
    
    va_end(args);
}

void print_header(const char *text) {
    print_colored(COLOR_BRIGHT_BLUE, "\n=== ");
    print_colored(COLOR_BRIGHT_WHITE, "%s", text);
    print_colored(COLOR_BRIGHT_BLUE, " ===\n");
}

void print_progress(const char *text) {
    print_colored(COLOR_BRIGHT_MAGENTA, ">> ");
    print_colored(COLOR_WHITE, "%s", text);
}
