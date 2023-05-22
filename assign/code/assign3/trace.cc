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

static string readString(pid_t pid, unsigned long addr) { 
  // addr is a char * read from an argument register via PTRACE_PEEKUSER  
  string str;
  // start out empty
  size_t numBytesRead = 0;
  while (true) {
    long ret = ptrace(PTRACE_PEEKDATA, pid, addr + numBytesRead);
    // code that analyzes the sizeof(long) bytes to see if there's a \0 inside    
    // code that extends str up to eight bytes in length, but possibly less 
    // if ret included a \0 byte
    char * s = (char *) &ret;
    for (size_t i=0; i < sizeof(long); i++) {
      char ch = s[i];
      if (ch == '\0') return str;
      str += ch;
    }
    numBytesRead += sizeof(long);
  }
}

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

  map<int, string> systemCallNumbers;
  map<string, int> systemCallNames;
  map<string, systemCallSignature> systemCallSignatures;
  map<int, string> errorConstants;
  int registers[] = {RDI, RSI, RDX, R10, R8, R9};

  if (!simple) {
    compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);
    compileSystemCallErrorStrings(errorConstants);
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
        systemCallSignature args = systemCallSignatures[syscall];
        cout << syscall << "(";
        int sz = (int) args.size();
        for (int i=0; i < sz; i++) {
          long arg = ptrace(PTRACE_PEEKUSER, pid, registers[i] * sizeof(long));
          switch (args[i]) {
            case SYSCALL_INTEGER: {
              cout << arg;
              break;
            }
            case SYSCALL_POINTER: {
              void *pts = (void *) arg;
              if (pts == NULL)
                cout << "NULL";
              else 
                cout << pts;
              break;
            }
            case SYSCALL_STRING: {
              string str = readString(pid, arg);
              cout << "\"" << str << "\"";
              break;
            }
            default:
              break;
          }
          if (i < sz - 1) cout << ", ";
        }
        cout << ") = " << flush;
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
        if (retval < 0) {
          cout << "-1 " << errorConstants[-retval] << " (" << strerror(-retval) << ")" << endl;
        } else {
          string syscall = systemCallNumbers[opcode];
          if (syscall != "brk" && syscall != "mmap") {
            cout << retval << endl;
          } else {
            cout << (void *) retval << endl;
          }
        }
      }
    } 
  }
  cout << "Program exited normally with status " << WEXITSTATUS(status) << endl;
  return WEXITSTATUS(status);
}