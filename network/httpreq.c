#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
  do {                                                                         \
    fprintf(stderr, "%s: %s (%s:%d)\n", (msg), strerror(errno), __FILE__,      \
            __LINE__);                                                         \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define handle_gai_error(msg, gai_err)                                         \
  do {                                                                         \
    fprintf(stderr, "%s: %s (%s:%d)\n", (msg), gai_strerror(gai_err),          \
            __FILE__, __LINE__);                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Maximum timeout for a single call to connect()
#define TIMEOUT_MAX 100

// Attempt to connect to service described in ai, timeout after timeout_ms
// Returns file descriptor on success, -1 on failure (errno is set)
int conn_with_timeout(struct addrinfo *ai, unsigned int timeout_ms)
{
  int sockfd;
  int sockfdflags_old;
  int pollret;

  if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) ==
      -1) {
    handle_error("socket");
  }

  // Timeout impl requires non-blocking socket
  if ((sockfdflags_old = fcntl(sockfd, F_GETFL, 0)) == -1) {
    handle_error("fcntl");
  }
  if (fcntl(sockfd, F_SETFL, sockfdflags_old | O_NONBLOCK) == -1) {
    handle_error("fcntl");
  }

  if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) {
    if ((errno != EWOULDBLOCK) && (errno != EINPROGRESS)) {
      close(sockfd);
      sockfd = -1;
      return sockfd;
    }
  }

  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
    handle_error("clock_gettime");
  }

  struct timespec deadline = {.tv_sec = now.tv_sec,
                              .tv_nsec = now.tv_nsec + timeout_ms * 1000000l};

  // Use poll() to put process to sleep until connection is established
  // We loop here in case poll() is interrupted
  do {
    // Get current time and compare with deadline
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
      handle_error("clock_gettime");
    }
    int delta_ms = (int)((deadline.tv_nsec - now.tv_nsec) / 1000l +
                         (deadline.tv_sec - now.tv_sec) / 1000000l);
    if (delta_ms < 0) {
      // Timed out
      close(sockfd);
      errno = ETIMEDOUT;
      return -1;
    }

    struct pollfd pfds[] = {{.fd = sockfd, .events = POLLOUT}};
    pollret = poll(pfds, 1, timeout_ms);

  } while (pollret == -1 && errno == EINTR);

  // Figure out what happened when poll() returned
  if (pollret < 0) {
    // poll() encountered an error
    close(sockfd);
    return -1;
  }
  else if (pollret == 0) {
    // poll() timed out
    close(sockfd);
    errno = ETIMEDOUT;
    return -1;
  }
  else {
    // poll() told us our socket is ready
    // Do additional verification of socket
    int error = 0;
    socklen_t len = sizeof error;
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
      close(sockfd);
      return -1;
    }
    if (error != 0) {
      close(sockfd);
      errno = error;
      return -1;
    }
  }

  // Reset socket to default file status (blocking)
  if (fcntl(sockfd, F_SETFL, sockfdflags_old) == -1) {
    close(sockfd);
    return -1;
  }

  return sockfd;
}

// Attempt to connect to service described in ai
// Returns file descriptor if successful, -1 on error
int establish_conn(struct addrinfo *ai)
{
  struct addrinfo *cur = ai;

  while (cur != NULL) {}
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s address\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int gai_err;
  struct addrinfo *ai;

  if ((gai_err = getaddrinfo(argv[1], "http", NULL, &ai)) != 0) {
    handle_gai_error("getadrinfo", gai_err);
  }

  int sockfd;
  if ((sockfd = conn_with_timeout(ai, 1000)) == -1) {
    handle_error("conn_with_timeout");
  }

  printf("Connected!\n");

  const char *request = "GET / HTTP/1.1\r\nConnection: close\r\n\r\n";

  if (send(sockfd, request, strlen(request), 0) == -1) {
    handle_error("send");
  }

  char buf[256];
  size_t count;

  while ((count = recv(sockfd, buf, sizeof buf, 0)) > 0) {
    write(STDOUT_FILENO, buf, count);
  }

  close(sockfd);

  return 0;
}
