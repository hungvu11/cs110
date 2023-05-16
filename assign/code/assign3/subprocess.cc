/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include <errno.h>
#include "subprocess.h"
using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
    int fds[2];
    pipe(fds);
    subprocess_t process = { fork(), fds[1], fds[0] };
    if (process.pid == 0) { 
        close(fds[1]); // no writing necessary
        if (supplyChildInput == true) {
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]); // already duplicated
        }
        execvp(argv[0], argv);
    }
    close(fds[0]); // not used in parent
    return process;
}
