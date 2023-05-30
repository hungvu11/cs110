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
#include <assert.h>
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

static void handleFgBuiltin(const pipeline& pipeline) {
  command cmd = pipeline.commands[0];
  size_t numOfToken = 0;
  for (size_t i=0; i < kMaxArguments; i++) {
    if (!cmd.tokens[i]) break;
    numOfToken++;
  }
  if (numOfToken != 1) throw STSHException("Correct Format: fg [job number]");

  try {
    int job_num = stoi(cmd.tokens[0]);

    if (job_num <= 0 || !joblist.containsJob(job_num)) throw STSHException("invalid job number");
    STSHJob& job = joblist.getJob(job_num);
    
    kill(-job.getGroupID(), SIGCONT);
    if (job.getState() == kBackground) {
      job.setState(kForeground);
    }
  } catch (invalid_argument& ia) {
    throw STSHException("Job number must be an integer");
  }
}

static void handleBgBuiltin(const pipeline& pipeline) {
  command cmd = pipeline.commands[0];
  size_t numOfToken = 0;
  for (size_t i=0; i < kMaxArguments; i++) {
    if (!cmd.tokens[i]) break;
    numOfToken++;
  }
  if (numOfToken != 1) throw STSHException("Correct Format: bg [job number]");

  try {
    int job_num = stoi(cmd.tokens[0]);

    if (job_num <= 0 || !joblist.containsJob(job_num)) throw STSHException("invalid job number");
    STSHJob& job = joblist.getJob(job_num);
    
    kill(-job.getGroupID(), SIGCONT);
    if (job.getState() == kForeground) {
      job.setState(kBackground);
    }
  } catch (invalid_argument& ia) {
    throw STSHException("Job number must be an integer");
  }
}

static void handleSlayBuiltin(const pipeline& pipeline) {
  command cmd = pipeline.commands[0];
  size_t numOfToken = 0;
  for (size_t i=0; i < kMaxArguments; i++) {
    if (!cmd.tokens[i]) break;
    numOfToken++;
  }
  if (numOfToken != 1 && numOfToken != 2) throw STSHException("Correct Format: slay [process pid] or slay [job number] [process index]");

  try {
    if (numOfToken == 2) {
      int job_num = stoi(cmd.tokens[0]);
      size_t process_index = stoi(cmd.tokens[1]);
      if (job_num <= 0 || !joblist.containsJob(job_num)) throw STSHException("invalid job number");
      STSHJob& job = joblist.getJob(job_num);
      const vector<STSHProcess> processes = job.getProcesses();
      if (process_index < 0 || process_index >= processes.size()) throw STSHException("invalid process index");
      
      STSHProcess process = processes[process_index];
      kill(process.getID(), SIGKILL);
    } else {
      int pid = stoi(cmd.tokens[0]);
      if (joblist.containsProcess(pid) == true) {
        kill(pid, SIGKILL);
      } else STSHException("not found process!");
    }
  } catch (invalid_argument& ia) {
    throw STSHException("Invalid arguments");
  }
}

static void handleHaltBuiltin(const pipeline& pipeline) {
  command cmd = pipeline.commands[0];
  size_t numOfToken = 0;
  for (size_t i=0; i < kMaxArguments; i++) {
    if (!cmd.tokens[i]) break;
    numOfToken++;
  }
  if (numOfToken != 1 && numOfToken != 2) throw STSHException("Correct Format: halt [process pid] or halt [job number] [process index]");

  try {
    if (numOfToken == 2) {
      int job_num = stoi(cmd.tokens[0]);
      size_t process_index = stoi(cmd.tokens[1]);
      if (job_num <= 0 || !joblist.containsJob(job_num)) throw STSHException("invalid job number");
      STSHJob& job = joblist.getJob(job_num);
      const vector<STSHProcess> processes = job.getProcesses();
      if (process_index < 0 || process_index >= processes.size()) throw STSHException("invalid process index");
      
      STSHProcess process = processes[process_index];
      kill(process.getID(), SIGSTOP);
    } else {
      int pid = stoi(cmd.tokens[0]);
      if (joblist.containsProcess(pid) == true) {
        kill(pid, SIGSTOP);
      } else STSHException("not found process!");
    }
  } catch (invalid_argument& ia) {
    throw STSHException("Invalid arguments");
  }
}

static void handleContBuiltin(const pipeline& pipeline) {
  command cmd = pipeline.commands[0];
  size_t numOfToken = 0;
  for (size_t i=0; i < kMaxArguments; i++) {
    if (!cmd.tokens[i]) break;
    numOfToken++;
  }
  if (numOfToken != 1 && numOfToken != 2) throw STSHException("Correct Format: cont [process pid] or cont [job number] [process index]");

  try {
    if (numOfToken == 2) {
      int job_num = stoi(cmd.tokens[0]);
      size_t process_index = stoi(cmd.tokens[1]);
      if (job_num <= 0 || !joblist.containsJob(job_num)) throw STSHException("invalid job number");
      STSHJob& job = joblist.getJob(job_num);
      const vector<STSHProcess> processes = job.getProcesses();
      if (process_index < 0 || process_index >= processes.size()) throw STSHException("invalid process index");
      
      STSHProcess process = processes[process_index];
      kill(process.getID(), SIGCONT);
    } else {
      int pid = stoi(cmd.tokens[0]);
      if (joblist.containsProcess(pid) == true) {
        kill(pid, SIGCONT);
      } else STSHException("not found process!");
    }
  } catch (invalid_argument& ia) {
    throw STSHException("Invalid arguments");
  }
}

static bool handleBuiltin(const pipeline& pipeline) {
  const string& command = pipeline.commands[0].command;
  auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
  if (iter == kSupportedBuiltins + kNumSupportedBuiltins) return false;
  size_t index = iter - kSupportedBuiltins;

  switch (index) {
  case 0:
  case 1: exit(0);
  case 2: handleFgBuiltin(pipeline); break;
  case 3: handleBgBuiltin(pipeline); break;
  case 4: handleSlayBuiltin(pipeline); break;
  case 5: handleHaltBuiltin(pipeline); break;
  case 6: handleContBuiltin(pipeline); break;
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
// /* static */ void addToJobList(STSHJobList& jobList, const vector<pair<pid_t, string>>& children) {
//   STSHJob& job = jobList.addJob(kBackground); //
//   for (const pair<string, pid_t>& child: children) {
//     pid_t pid = child.first;
//     const string& command = child.second;
//     job.addProcess(STSHProcess(pid, command)); // third argument defaults to kRunning     
//   }
  
//   cout << jobList;
// }
static void updateJobList(STSHJobList& jobList, pid_t pid, STSHProcessState state) {
  if (!jobList.containsProcess(pid)) return;
  STSHJob& job = jobList.getJobWithProcess(pid);
  assert(job.containsProcess(pid));
  STSHProcess& process = job.getProcess(pid);
  process.setState(state);
  jobList.synchronize(job);
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
  int status;

  pid_t pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
  if (pid <= 0) return;
  if (pid == fgpid) fgpid = 0; // clear foreground process
  if (WIFSTOPPED(status)) updateJobList(joblist, pid, kStopped);
  else if (WIFCONTINUED(status)) updateJobList(joblist, pid, kRunning);
  else updateJobList(joblist, pid, kTerminated);

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
    setpgid(0, 0);
    command cmd = p.commands[0];
    char* argv[kMaxArguments + 2];
    argv[0] = cmd.command;
    for (size_t i=0; i<=kMaxArguments; i++) {
      argv[i+1] = cmd.tokens[i];
    }
    execvp(cmd.command, argv);
  }
  
  if (!p.background) {
    STSHJob& job = joblist.addJob(kForeground);
    job.addProcess(STSHProcess(pid, p.commands[0]));
    waitForForegroundProcess(pid);
  } else {
    STSHJob& job = joblist.addJob(kBackground);
    job.addProcess(STSHProcess(pid, p.commands[0]));
    unblockSIGCHLD();
  }
  cout << joblist;
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