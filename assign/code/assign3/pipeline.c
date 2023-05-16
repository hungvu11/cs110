/**
 * File: pipeline.c
 * ----------------
 * Presents the implementation of the pipeline routine.
 */

#include "pipeline.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]) {
    int fds[2];
    pipe(fds);
    pid_t read = fork();
    if (read == 0) {
        // child
        close(fds[1]); // no need to write
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        execvp(argv2[0], argv2);
    }
    pids[1] = read;

    pid_t write = fork();
    if (write == 0) {
        // child
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        execvp(argv1[0], argv1);
    }
    pids[0] = write;
    close(fds[0]);
    close(fds[1]);
}
