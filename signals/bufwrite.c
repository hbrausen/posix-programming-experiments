#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 1000000000

char gBuffer[BUFSIZE];

FILE *mk_temp_file()
{
  int fd;
  FILE *fp;

  char template[] = "/tmp/bufwXXXXXX";

  if ((fd = mkstemp(template)) == -1) {
    perror("could not create temporary file");
    return NULL;
  }

  unlink(template);

  if ((fp = fdopen(fd, "w")) == NULL) {
    perror("could not open file descriptor");
    return NULL;
  }

  return fp;
}

void sigalrm_handler(int signo) { printf("SIGALRM\n"); }

int main(int argc, char *argv[])
{
  FILE *fp;
  struct sigaction sa;

  //if ((fp = mk_temp_file()) == NULL) {
  //  exit(EXIT_FAILURE);
  //}

  if ((fp = tmpfile()) == NULL) {
    perror("could not create temporary file");
    exit(EXIT_FAILURE);
  }

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigalrm_handler;

  if (sigaction(SIGALRM, &sa, NULL) == -1) {
    perror("could not assign signal handler");
    exit(EXIT_FAILURE);
  }

  alarm(1);

  printf("Starting fwrite...\n");
  int ret = fwrite(gBuffer, sizeof(gBuffer[0]),
                   sizeof(gBuffer) / sizeof(gBuffer[0]), fp);
  printf("fwrite returned %d\n", ret);

  if (ret < sizeof(gBuffer) / sizeof(gBuffer[0])) {
    printf("Partial write.\n");

    if (feof(fp)) {
      printf("eof\n");
    }

    if (ferror(fp)) {
      printf("error\n");
    }
  }

  fclose(fp);

  return 0;
}
