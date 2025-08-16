#include "commands.h"
#include "process_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#define DEFAULT_SERVER_PATH "."
#define DEFAULT_CONFIG_PATH "config.json"

#ifdef _WIN32
#define SERVER_EXECUTABLE "omp-server.exe"
#else
#define SERVER_EXECUTABLE "omp-server"
#endif

// Global variables for server process handling
#ifdef _WIN32
static HANDLE g_server_process = NULL;
#else
static pid_t g_server_pid = 0;
#endif
static volatile int g_running = 0;

// Signal handler for CTRL+C
static void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nReceived termination signal. Shutting down server...\n");
        g_running = 0;
        
#ifdef _WIN32
        if (g_server_process != NULL) {
            TerminateProcess(g_server_process, 0);
            CloseHandle(g_server_process);
            g_server_process = NULL;
        }
#else
        if (g_server_pid > 0) {
            kill(g_server_pid, SIGTERM);
            g_server_pid = 0;
        }
#endif
    }
}

/**
 * Print run command usage
 */
static void print_run_usage(void) {
    printf("Usage: opencli run [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --server-path <path>    Path to the server directory (default: %s)\n", DEFAULT_SERVER_PATH);
    printf("  --config <path>         Path to server config file (default: %s)\n", DEFAULT_CONFIG_PATH);
    printf("  --help                  Show this help message\n");
    printf("\n");
    printf("Press Ctrl+C to stop the server\n");
}

int command_run(int argc, char *argv[]) {
    
#ifdef __ANDROID__
    fprintf(stderr, "Error: open.mp server cannot run on Termux/Android environment\n");
    fprintf(stderr, "Termux does not support server execution due to system limitations\n");
    return EXIT_FAILURE;
#endif

    // Default values
    char server_path[512] = DEFAULT_SERVER_PATH;
    char config_path[512] = DEFAULT_CONFIG_PATH;
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_run_usage();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--server-path") == 0 && i + 1 < argc) {
            #ifdef _WIN32
            strcpy_s(server_path, sizeof(server_path), argv[++i]);
            #else
            strcpy(server_path, argv[++i]);
            #endif
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            #ifdef _WIN32
            strcpy_s(config_path, sizeof(config_path), argv[++i]);
            #else
            strcpy(config_path, argv[++i]);
            #endif
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_run_usage();
            return EXIT_FAILURE;
        }
    }
    
    // Prepare server path
    char server_exe[512];
#ifdef _WIN32
    sprintf_s(server_exe, sizeof(server_exe), "%s\\%s", server_path, SERVER_EXECUTABLE);
#else
    sprintf(server_exe, "%s/%s", server_path, SERVER_EXECUTABLE);
#endif

    // Check if server is already running
    if (is_process_running(SERVER_EXECUTABLE)) {
        fprintf(stderr, "Error: open.mp server is already running\n");
        return EXIT_FAILURE;
    }
    
    printf("Starting open.mp server from %s with config %s\n", server_exe, config_path);
    printf("Press Ctrl+C to stop the server\n");
    
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    
    char *args[4];
    args[0] = server_exe;  
    args[1] = "--config";
    args[2] = config_path;
    args[3] = NULL;
    
    
#ifdef _WIN32
    // Windows implementation
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    
    char cmd_line[1024] = "";
    strcat_s(cmd_line, sizeof(cmd_line), server_exe);
    
    for (int i = 1; args[i] != NULL; i++) {
        strcat_s(cmd_line, sizeof(cmd_line), " ");
        strcat_s(cmd_line, sizeof(cmd_line), args[i]);
    }
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed (%lu)\n", GetLastError());
        return EXIT_FAILURE;
    }
    
    
    g_server_process = pi.hProcess;
    g_running = 1;
    
    
    printf("Server started successfully! Waiting for Ctrl+C to terminate...\n");
    while (g_running) {
        Sleep(100);  
    }
    
    CloseHandle(pi.hThread);
    printf("Server terminated.\n");
#else
    
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        
        execvp(server_exe, args);
        fprintf(stderr, "execvp failed\n");
        exit(EXIT_FAILURE);
    } else {
        
        g_server_pid = pid;
        g_running = 1;
        
        printf("Server started successfully! Waiting for Ctrl+C to terminate...\n");
        while (g_running) {
            usleep(100000);  
        }
        
        printf("Server terminated.\n");
    }
#endif
    
    return EXIT_SUCCESS;
} 