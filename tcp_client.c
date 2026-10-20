// Simple TCP echo client. Run tcp_server first in another terminal.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8080

static double seconds_between(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect"); close(fd); return 1;
    }

    const char *msg = "Hello from TCP client";
    if (send(fd, msg, strlen(msg), 0) < 0) {
        perror("send"); close(fd); return 1;
    }
    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    if (n < 0) { perror("recv"); close(fd); return 1; }
    buf[n] = '\0';

    printf("Reply: %s\n", buf);
    printf("Request/reply elapsed time (approx RTT): %.3f ms\n",
           seconds_between(start, end) * 1000.0);
    printf("Received application bytes: %zd\n", n);
    close(fd);
    return 0;
}
