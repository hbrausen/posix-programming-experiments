#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
  if (fork() != 0) {
    exit(0);
  }
  sleep(1);
  setsid();
  close(0);
  close(1);
  close(2);
  if (fork() != 0) {
    exit(0);
  }
  sleep(2);
  syslog(LOG_INFO, "Still alive!");

  return 0;
}
