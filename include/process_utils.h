#ifndef OPENCLI_PROCESS_UTILS_H
#define OPENCLI_PROCESS_UTILS_H

#include <stdbool.h>

/**
 * Run a process with given command and arguments
 */
int run_process(const char *command, char *const args[], bool wait_for_exit);

/**
 * Check if a process with given name is running
 */
bool is_process_running(const char *process_name);

#endif 