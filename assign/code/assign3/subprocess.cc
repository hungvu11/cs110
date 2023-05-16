/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include <errno.h>
#include "subprocess.h"
using namespace std;

void sp_pipe(int fds[2]) throw (SubprocessException) {
  if (pipe(fds) == 0) return;
  switch errno {
    case EFAULT:
      throw SubprocessException("The fds buffer is in an invalid area of the process's address space.\n");
    case EMFILE:
      throw SubprocessException("Too many descriptors are active.\n");
    case ENFILE:
      throw SubprocessException("The system file table is full.\n");
  }
}

int sp_fork() throw (SubprocessException){
  int pid = fork();
  if (pid >= 0) return pid;
  switch errno {
    case EAGAIN:
      throw SubprocessException("The system-imposed limit on the total number of processes under execution would be exceeded.\n");
    case ENOMEM:
      throw SubprocessException("There is insufficient swap space for the new process.\n");
  }
  return -1;
}

void sp_close(int fd) {
  if (close(fd) == 0) return;
  switch errno {
    case EBADF:
      throw SubprocessException("fd is not a valid, active file descriptor.\n");
    case EINTR:
      throw SubprocessException("The execution of close was interrupted by a signal.\n");
    case EIO:
      throw SubprocessException("A previously-uncommitted write(2) encountered an input/output error.\n");
  }
}

void sp_dup2(int fd, int fd2) {
  if (dup2(fd, fd2) == 0) return;
  switch errno {
    case EBADF:
      throw SubprocessException("fd and/or fd2 is not an active, valid file descriptor.\n");
    case EINTR:
      throw SubprocessException("Execution is interrupted by a signal.\n");
    case EMFILE:
      throw SubprocessException("Too many file descriptors are active.\n");
  }
}

void sp_execvp(const char *file, char *const argv[]) {
  execvp(file, argv);
  switch errno {
    case E2BIG:
      throw SubprocessException("The number of bytes in the new process's argument list is larger than the system-imposed limit.\n");
    case EACCES:
      throw SubprocessException("One of the following errors have occured: "
                                    "1. Search permission is denied for a component of the path prefix."
                                    "2. The new process file is not an ordinary file."
                                    "3. The new process file mode denies execute permission."
                                    "4. The new process file is on a filesystem mounted with execution disabled\n");
    case EFAULT:
      throw SubprocessException("One of the following errors have occured: "
                                    "1. The new process file is not as long as indicated by the size values in its header."
                                    "2. file or argv point to an illegal address.\n");
    case EIO:
      throw SubprocessException("An I/O error occurred while reading from the file system.\n");
    case ELOOP:
      throw SubprocessException("Too many symbolic links were encountered in translating the pathname."
                                    "This is taken to be indicative of a looping symbolic link.\n");
    case ENAMETOOLONG:
      throw SubprocessException("A component of a pathname exceeded {NAME_MAX} characters,"
                                    " or an entire path name exceeded {PATH_MAX} characters.\n");
    case ENOENT:
      throw SubprocessException("The new process file does not exist.\n");
    case ENOEXEC:
      throw SubprocessException("The new process file has the appropriate access permission, "
                                    "but has an unrecognized for mat (e.g., an invalid magic number in its header).\n");
    case ENOMEM:
      throw SubprocessException("The new process requires more virtual memory than is allowed by the imposed maximum (getrlimit(2)).\n");
    case ENOTDIR:
      throw SubprocessException("A component of the path prefix is not a directory.\n");
    case ETXTBSY:
      throw SubprocessException("The new process file is a pure procedure (shared text) file "
                                    "that is currently open for writing or reading by some process.\n");

  }
}

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {

}
