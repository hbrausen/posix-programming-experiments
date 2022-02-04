#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

int daemonize()
{
  pid_t pid;
  struct sigaction sa;
  struct rlimit rl;
  int fd[3];

  // Clear file creation mask
  umask(0);

  // Get open file limit
  if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
    return -1;
  }

  // Ignore SIGHUP
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGHUP, &sa, NULL) == -1) {
    return -1;
  }

  // Fork and exit parent
  if ((pid = fork()) == -1) {
    return -1;
  }

  if (pid != 0) {
    exit(0);
  }

  if (chdir("/") == -1) {
    return -1;
  }

  // Close all open files
  if (rl.rlim_max == RLIM_INFINITY) {
    rl.rlim_max = 1024;
  }

  for (rlim_t i = 0; i < rl.rlim_max; ++i) {
    close(i);
  }

  // Create new session
  if (setsid() == -1) {
    return -1;
  }

  // Fork and exit parent
  if ((pid = fork()) == -1) {
    return -1;
  }

  if (pid != 0) {
    exit(0);
  }

  // Open /dev/null on fd 0, 1, 2
  if ((fd[0] = open("/dev/null", 0)) == -1) {
    return -1;
  }

  fd[1] = dup(fd[0]);
  fd[2] = dup(fd[0]);

  openlog("daemond", LOG_NDELAY, LOG_DAEMON);
  if (fd[0] != 0 || fd[1] != 1 || fd[2] != 2) {
    syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd[0], fd[1],
           fd[2]);
    exit(1);
  }
}

int main(int argc, char *argv[])
{
  printf("Daemonizing...\n");
  daemonize();
  syslog(LOG_INFO, "I am now a daemon!");
  sleep(5);
  syslog(LOG_INFO, "Exiting...");

  return 0;
}
