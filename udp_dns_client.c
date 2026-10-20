// UDP DNS client with a small local cache and receive timeout.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 5353
#define BUFSIZE 256

int main(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct timeval tv = {2, 0}; // 2-second timeout
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char cache_host[BUFSIZE] = "";
    char cache_ip[BUFSIZE] = "";

    for (;;) {
        char host[BUFSIZE], reply[BUFSIZE];
        printf("Hostname (or quit): ");
        if (!fgets(host, sizeof(host), stdin)) break;
        host[strcspn(host, "\r\n")] = '\0';
        if (strcmp(host, "quit") == 0) break;
        if (host[0] == '\0' || strchr(host, ' ') != NULL) {
            puts("Invalid hostname format."); continue;
        }

        if (strcmp(host, cache_host) == 0) {
            printf("Cache hit: %s -> %s\n", cache_host, cache_ip);
            continue;
        }

        if (sendto(fd, host, strlen(host), 0,
                   (struct sockaddr *)&server, sizeof(server)) < 0) {
            perror("sendto"); continue;
        }
        ssize_t n = recvfrom(fd, reply, sizeof(reply)-1, 0, NULL, NULL);
        if (n < 0) {
            puts("Timeout: no DNS response received."); continue;
        }
        reply[n] = '\0';
        printf("Server reply: %s\n", reply);
        if (strcmp(reply, "NXDOMAIN") != 0) {
            snprintf(cache_host, sizeof(cache_host), "%s", host);
            snprintf(cache_ip, sizeof(cache_ip), "%s", reply);
        }
    }
    close(fd);
    return 0;
}
