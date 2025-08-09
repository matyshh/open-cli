#include "process_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

int run_process(const char *command, char *const args[], bool wait_for_exit) {
#ifdef _WIN32
    // Windows implementation
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    // Prepare command line
    char cmd_line[1024] = "";
    strcat_s(cmd_line, sizeof(cmd_line), command);
    
    for (int i = 0; args[i] != NULL; i++) {
        strcat_s(cmd_line, sizeof(cmd_line), " ");
        strcat_s(cmd_line, sizeof(cmd_line), args[i]);
    }
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed (%lu)\n", GetLastError());
        return -1;
    }
    
    if (wait_for_exit) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exit_code = 0;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (int)exit_code;
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }
#else
    // POSIX implementation
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return -1;
    } else if (pid == 0) {
        execvp(command, args);
        fprintf(stderr, "execvp failed\n");
        exit(EXIT_FAILURE);
    } else {
        if (wait_for_exit) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return -1;
            }
        }
        return 0;
    }
#endif
}

bool is_process_running(const char *process_name) {
#ifdef _WIN32
    // Windows implementation
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(snapshot, &pe32)) {
        CloseHandle(snapshot);
        return false;
    }
    
    do {
        if (strcmp(pe32.szExeFile, process_name) == 0) {
            CloseHandle(snapshot);
            return true;
        }
    } while (Process32Next(snapshot, &pe32));
    
    CloseHandle(snapshot);
    return false;
#else
    // POSIX implementation (simplified)
    char cmd[256];
    #ifdef _WIN32
    sprintf_s(cmd, sizeof(cmd), "pgrep -x %s > /dev/null", process_name);
    #else
    sprintf(cmd, "pgrep -x %s > /dev/null", process_name);
    #endif
    return system(cmd) == 0;
#endif
} 