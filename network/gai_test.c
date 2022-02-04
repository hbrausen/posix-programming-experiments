#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

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

// Print formatted contents of addrinfo struct to stdout
void print_addrinfo(struct addrinfo *ai)
{
  printf("ai_flags:");
  if (ai->ai_flags & AI_PASSIVE) printf(" AI_PASSIVE");
  if (ai->ai_flags & AI_NUMERICSERV) printf(" AI_NUMERICSERV");
  if (ai->ai_flags & AI_CANONNAME) printf(" AI_CANONNAME");
  if (ai->ai_flags & AI_ADDRCONFIG) printf(" AI_ADDRCONFIG");
  if (ai->ai_flags & AI_V4MAPPED) printf(" AI_V4MAPPED");
  if (ai->ai_flags & AI_NUMERICHOST) printf(" AI_NUMERICHOST");
  if (ai->ai_flags & AI_ALL) printf(" AI_ALL");
  printf("\n");
  printf("ai_family: ");
  switch (ai->ai_family) {
  case AF_INET: printf("AF_INET"); break;
  case AF_INET6: printf("AF_INET6"); break;
  case AF_UNIX: printf("AF_UNIX"); break;
  case AF_UNSPEC: printf("AF_UNSPEC"); break;
  default: printf("unknown (%d)", ai->ai_family);
  }
  printf("\n");
  printf("ai_socktype: ");
  switch (ai->ai_socktype) {
  case SOCK_DGRAM: printf("SOCK_DGRAM"); break;
  case SOCK_RAW: printf("SOCK_RAW"); break;
  case SOCK_SEQPACKET: printf("SOCK_SEQPACKET"); break;
  case SOCK_STREAM: printf("SOCK_STREAM"); break;
  default: printf("unknown (%d)", ai->ai_socktype);
  }
  printf("\n");
  printf("ai_protocol: ");
  switch (ai->ai_protocol) {
  case 0: printf("default"); break;
  case IPPROTO_TCP: printf("IPPROTO_TCP"); break;
  case IPPROTO_UDP: printf("IPPROTO_UDP"); break;
  case IPPROTO_RAW: printf("IPPROTO_RAW"); break;
  default: printf("unknown (%d)", ai->ai_protocol);
  }
  printf("\n");
  printf("ai_addrlen: %d\n", ai->ai_addrlen);
  printf("ai_addr: ");
  char buf[INET6_ADDRSTRLEN];
  switch (ai->ai_family) {
  case AF_INET:
    printf("%s", inet_ntop(ai->ai_family,
                           &((struct sockaddr_in *)ai->ai_addr)->sin_addr, buf,
                           ai->ai_addrlen));
    break;
  case AF_INET6:
    printf("%s", inet_ntop(ai->ai_family,
                           &((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr,
                           buf, ai->ai_addrlen));
    break;
  default: printf("unknown");
  }
  printf(" port %d\n", ntohs(((struct sockaddr_in *)ai->ai_addr)->sin_port));
  printf("ai_canonname: %s\n", ai->ai_canonname);
}

int main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("Usage: %s node service\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int gai_err;
  struct addrinfo *ai;
  struct addrinfo hint;
  memset(&hint, 0, sizeof(hint));
  hint.ai_flags = AI_CANONNAME;
  if ((gai_err = getaddrinfo(argv[1], argv[2], &hint, &ai)) != 0) {
    handle_gai_error("getaddrinfo", gai_err);
  }

  for (struct addrinfo *cur = ai; cur != NULL; cur = cur->ai_next) {
    printf("addrinfo entry:\n");
    print_addrinfo(cur);
  }

  freeaddrinfo(ai);

  return 0;
}

