/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>  // for fork
#include <signal.h>  // for kill
#include <sys/wait.h>
using namespace std;

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it
static pid_t fgpid = 0;
/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins)/sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline& pipeline) {
  const string& command = pipeline.commands[0].command;
  auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
  if (iter == kSupportedBuiltins + kNumSupportedBuiltins) return false;
  size_t index = iter - kSupportedBuiltins;

  switch (index) {
  case 0:
  case 1: exit(0);
  case 7: cout << joblist; break;
  default: throw STSHException("Internal Error: Builtin command not supported."); // or not implemented yet
  }
  
  return true;
}

static void toggleSIGCHLDBlock(int how) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(how, &mask, NULL);
}

void blockSIGCHLD() {
  toggleSIGCHLDBlock(SIG_BLOCK);
}

void unblockSIGCHLD() {
  toggleSIGCHLDBlock(SIG_UNBLOCK);
}

static void waitForForegroundProcess(pid_t pid) {
  fgpid = pid;
  sigset_t empty;
  sigemptyset(&empty);
  while (fgpid == pid) {
    sigsuspend(&empty);
  }
  unblockSIGCHLD();
}

static void reapChild(int sig) {
  while (true) {
    pid_t pid = waitpid(-1, NULL, WNOHANG);
    if (pid <= 0) break;
    if (pid == fgpid) fgpid = 0; // clear foreground process
  }
}

static void handleSIGINT(int sig) {
  if (joblist.containsProcess(fgpid))
    kill(fgpid, SIGINT);
}

static void handleSIGTSPT(int sig) {
  if (joblist.containsProcess(fgpid))
    kill(fgpid, SIGTSTP);
}
/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD, 
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and 
 * ignores two others.
 */
static void installSignalHandlers() {
  installSignalHandler(SIGQUIT, [](int sig) { exit(0); });
  installSignalHandler(SIGTTIN, SIG_IGN);
  installSignalHandler(SIGTTOU, SIG_IGN);
  installSignalHandler(SIGCHLD, reapChild);
  installSignalHandler(SIGINT, handleSIGINT);
  installSignalHandler(SIGTSTP, handleSIGTSPT);
}

/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline& p) {
  blockSIGCHLD();
  pid_t pid = fork();
  if (pid == 0) { //child 
    unblockSIGCHLD();
    command cmd = p.commands[0];
    char* argv[kMaxArguments + 2];
    argv[0] = cmd.command;
    for (size_t i=0; i<=kMaxArguments; i++) {
      argv[i+1] = cmd.tokens[i];
    }
    execvp(cmd.command, argv);
  }
  STSHJob& job = joblist.addJob(kForeground);
  job.addProcess(STSHProcess(pid, p.commands[0]));
  cout << joblist;
  setpgid(pid, 0);
  waitForForegroundProcess(pid);
  STSHProcess& process = job.getProcess(pid);
  process.setState(kTerminated);
  joblist.synchronize(job);
}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).  
 */
int main(int argc, char *argv[]) {
  pid_t stshpid = getpid();
  installSignalHandlers();
  rlinit(argc, argv);
  while (true) {
    string line;
    if (!readline(line)) break;
    if (line.empty()) continue;
    try {
      pipeline p(line);
      bool builtin = handleBuiltin(p);
      if (!builtin) createJob(p);
    } catch (const STSHException& e) {
      cerr << e.what() << endl;
      if (getpid() != stshpid) exit(0); // if exception is thrown from child process, kill it
    }
  }

  return 0;
}