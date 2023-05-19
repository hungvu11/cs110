/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include <errno.h>
#include <fcntl.h>
#include "subprocess.h"
using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
    int supplyfd[2], ingestfd[2];
    pipe(supplyfd);
    pipe(ingestfd);

    subprocess_t process = {
        fork(),
        (supplyChildInput) ? supplyfd[1] : kNotInUse,
        (ingestChildOutput) ? ingestfd[0] : kNotInUse
    };

    if (process.pid > 0) {
        // parent
        close(supplyfd[0]);
        close(ingestfd[1]);
        return process;
    }

    // child

    close(supplyfd[1]);
    close(ingestfd[0]);
    
    if (supplyChildInput) dup2(supplyfd[0], STDIN_FILENO);
    if (ingestChildOutput) dup2(ingestfd[1], STDOUT_FILENO);

    close(supplyfd[0]);
    close(ingestfd[1]);

    execvp(argv[0], argv);
}
