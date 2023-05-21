/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unistd.h> // for fork, execvp
#include <string.h> // for memchr, strerror
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include "trace-options.h"
#include "trace-error-constants.h"
#include "trace-system-calls.h"
#include "trace-exception.h"
using namespace std;

int main(int argc, char *argv[]) {
  bool simple = false, rebuild = false;
  int numFlags = processCommandLineFlags(simple, rebuild, argv);
  if (argc - numFlags == 1) {
    cout << "Nothing to trace... exiting." << endl;
    return 0;
  }

  pid_t pid = fork();
  if (pid == 0) {
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP);
    execvp(argv[numFlags + 1], argv + numFlags + 1);
    return 0;
  }

  int status;
  waitpid(pid, &status, 0);
  assert(WIFSTOPPED(status));
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);

  map<int, std::string> systemCallNumbers;
  map<std::string, int> systemCallNames;
  map<std::string, systemCallSignature> systemCallSignatures;

  if (!simple) {
    compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);
  }

  while (true) {
    ptrace(PTRACE_SYSCALL, pid, 0, 0);
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      cout << "<no return>" << endl;
      break;
    } else if (WIFSTOPPED(status) && (WSTOPSIG(status) == (SIGTRAP | 0x80))) {
      int opcode = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));
      if (simple) {
        cout << "syscall(" << opcode << ") = " << flush;
      } else {
        string syscall = systemCallNumbers[opcode];
        systemCallSignature arg = systemCallSignatures[syscall];
      }

      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status, 0);

      if (WIFEXITED(status)) {
        cout << "<no return>" << endl;
        break;
      }
      long retval = ptrace(PTRACE_PEEKUSER, pid, RAX * sizeof(long));
      if (simple) {
        cout << retval << endl;
      } else {

      }
    } 
  }
  cout << "Program exited normally with status " << WEXITSTATUS(status) << endl;
  return WEXITSTATUS(status);
}