#include "process_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

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
#ifdef __ANDROID__
    if (access(command, F_OK) != 0 || access(command, X_OK) != 0) {
        fprintf(stderr, "Command not found or not executable: %s\n", command);
        return -1;
    }
        if (strstr(command, "pawncc") != NULL) {
            char source_lib_path[1024];
            char source_libpawnc_path[1024];
            char prefix_lib_path[1024];
            char prefix_libpawnc_path[1024];
            
            char *compiler_dir = strdup(command);
            char *last_slash = strrchr(compiler_dir, '/');
            
            if (last_slash != NULL) {
                *last_slash = '\0';  
                last_slash = strrchr(compiler_dir, '/');
                if (last_slash != NULL) {
                    *last_slash = '\0';  
                    
                    
                    snprintf(source_lib_path, sizeof(source_lib_path), "%s/lib", compiler_dir);
                    snprintf(source_libpawnc_path, sizeof(source_libpawnc_path), "%s/libpawnc.so", source_lib_path);
                    
                   
                    const char *prefix = getenv("PREFIX");
                    if (!prefix) {
                        prefix = "/data/data/com.termux/files/usr"; 
                    }
                    snprintf(prefix_lib_path, sizeof(prefix_lib_path), "%s/lib", prefix);
                    snprintf(prefix_libpawnc_path, sizeof(prefix_libpawnc_path), "%s/libpawnc.so", prefix_lib_path);
                    
                    if (access(source_libpawnc_path, R_OK) == 0) {
                        bool need_sync = true;
                        struct stat source_stat, dest_stat;
                        
                        if (stat(source_libpawnc_path, &source_stat) == 0 && 
                            stat(prefix_libpawnc_path, &dest_stat) == 0) {
                            
                            if (source_stat.st_size == dest_stat.st_size && 
                                source_stat.st_mtime == dest_stat.st_mtime) {
                                need_sync = false;
                            }
                        }
                        
                        if (need_sync) {
                            char lib_sync_cmd[2048];
                            snprintf(lib_sync_cmd, sizeof(lib_sync_cmd), "cp '%s' '%s' 2>/dev/null", source_libpawnc_path, prefix_libpawnc_path);
                            system(lib_sync_cmd);
                            

                            char source_pawncc_path[1024];
                            char prefix_bin_path[1024];
                            char prefix_pawncc_path[1024];
                            
                            snprintf(source_pawncc_path, sizeof(source_pawncc_path), "%s/bin/pawncc", compiler_dir);
                            snprintf(prefix_bin_path, sizeof(prefix_bin_path), "%s/bin", prefix);
                            snprintf(prefix_pawncc_path, sizeof(prefix_pawncc_path), "%s/pawncc", prefix_bin_path);
                            
                            if (access(source_pawncc_path, R_OK) == 0) {
                                char bin_sync_cmd[2048];
                                snprintf(bin_sync_cmd, sizeof(bin_sync_cmd), "cp '%s' '%s' 2>/dev/null && chmod +x '%s' 2>/dev/null", 
                                        source_pawncc_path, prefix_pawncc_path, prefix_pawncc_path);
                                system(bin_sync_cmd);
                            }
                            

                            char source_pawndisasm_path[1024];
                            char prefix_pawndisasm_path[1024];
                            
                            snprintf(source_pawndisasm_path, sizeof(source_pawndisasm_path), "%s/bin/pawndisasm", compiler_dir);
                            snprintf(prefix_pawndisasm_path, sizeof(prefix_pawndisasm_path), "%s/pawndisasm", prefix_bin_path);
                            
                            if (access(source_pawndisasm_path, R_OK) == 0) {
                                char disasm_sync_cmd[2048];
                                snprintf(disasm_sync_cmd, sizeof(disasm_sync_cmd), "cp '%s' '%s' 2>/dev/null && chmod +x '%s' 2>/dev/null", 
                                        source_pawndisasm_path, prefix_pawndisasm_path, prefix_pawndisasm_path);
                                system(disasm_sync_cmd);
                            }
                        }
                        

                        char new_path[2048];
                        const char *current_path = getenv("LD_LIBRARY_PATH");
                        if (current_path) {
                            snprintf(new_path, sizeof(new_path), "%s:%s", prefix_lib_path, current_path);
                            setenv("LD_LIBRARY_PATH", new_path, 1);
                        } else {
                            setenv("LD_LIBRARY_PATH", prefix_lib_path, 1);
                        }
                        
                        setenv("FORTIFY_SOURCE", "0", 1);
                        
                        fflush(stdout);
                        fflush(stderr);
                        
                        setenv("TERM", "xterm", 0);
                        setenv("LC_ALL", "C", 0);
                        
                    }
                }
            }
            free(compiler_dir);
        }
        
        char full_command[4096] = {0};
        strcat(full_command, command);  
        
        // Comprehensive FORTIFY bypass: Multiple approaches for NDK binaries
        char bypass_command[4096] = {0};
        
        // Method 1: Try with unset + export approach
        snprintf(bypass_command, sizeof(bypass_command), 
                "unset FORTIFY_SOURCE; unset _FORTIFY_SOURCE; "
                "export FORTIFY_SOURCE=0; export _FORTIFY_SOURCE=0; "
                "export __BIONIC_FORTIFY=0; export TERM=xterm; export LC_ALL=C; "
                "exec %s", command);
        
        // Build command with proper shell escaping for metacharacters
        for (int i = 1; args[i] != NULL; i++) {
            strcat(bypass_command, " ");
            
            // Check if argument contains shell metacharacters that need escaping
            char *arg = args[i];
            int needs_escaping = 0;
            for (char *p = arg; *p; p++) {
                if (*p == '(' || *p == ')' || *p == ';' || *p == '+' || *p == '\\' || *p == ' ') {
                    needs_escaping = 1;
                    break;
                }
            }
            
            if (needs_escaping) {
                // Use single quotes to preserve all metacharacters literally
                strcat(bypass_command, "'");
                strcat(bypass_command, arg);
                strcat(bypass_command, "'");
            } else {
                strcat(bypass_command, arg);
            }
        }
        
        // First attempt: Comprehensive bypass
        int result = system(bypass_command);
        
        // If still failing with FORTIFY (exit code 134 = SIGABRT), try fallback approaches
        if (WEXITSTATUS(result) == 134 || result == 34304) {
            
            // Method 2: Direct execution with timeout
            char fallback_command[4096] = {0};
            snprintf(fallback_command, sizeof(fallback_command), 
                    "timeout 30s %s", command);
            
            for (int i = 1; args[i] != NULL; i++) {
                strcat(fallback_command, " ");
                
                // Apply same escaping logic for fallback
                char *arg = args[i];
                int needs_escaping = 0;
                for (char *p = arg; *p; p++) {
                    if (*p == '(' || *p == ')' || *p == ';' || *p == '+' || *p == '\\' || *p == ' ') {
                        needs_escaping = 1;
                        break;
                    }
                }
                
                if (needs_escaping) {
                    strcat(fallback_command, "'");
                    strcat(fallback_command, arg);
                    strcat(fallback_command, "'");
                } else {
                    strcat(fallback_command, arg);
                }
            }
            
            result = system(fallback_command);
            
            // Method 3: If still failing, try with sh -c wrapper
            if (WEXITSTATUS(result) == 134 || result == 34304) {
                
                char wrapper_command[4096] = {0};
                snprintf(wrapper_command, sizeof(wrapper_command), 
                        "sh -c 'FORTIFY_SOURCE=0 exec %s", command);
                
                for (int i = 1; args[i] != NULL; i++) {
                    strcat(wrapper_command, " ");
                    
                    // Apply same escaping logic for sh -c wrapper
                    char *arg = args[i];
                    int needs_escaping = 0;
                    for (char *p = arg; *p; p++) {
                        if (*p == '(' || *p == ')' || *p == ';' || *p == '+' || *p == '\\' || *p == ' ' || *p == '\'') {
                            needs_escaping = 1;
                            break;
                        }
                    }
                    
                    if (needs_escaping) {
                        // Inside sh -c, we need to escape single quotes differently
                        strcat(wrapper_command, "\"");
                        strcat(wrapper_command, arg);
                        strcat(wrapper_command, "\"");
                    } else {
                        strcat(wrapper_command, arg);
                    }
                }
                strcat(wrapper_command, "'");
                
                result = system(wrapper_command);
            }
        }
        return WEXITSTATUS(result);
        
#else
    // Non-Android POSIX: Use traditional fork/execvp
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return -1;
    } else if (pid == 0) {
        if (access(command, F_OK) != 0 || access(command, X_OK) != 0) {
            exit(EXIT_FAILURE);
        }
        
        execvp(command, args);
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
    sprintf(cmd, "pgrep -x %s > /dev/null", process_name);
    return system(cmd) == 0;
#endif
} 