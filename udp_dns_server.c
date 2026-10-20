// UDP-based DNS teaching simulation. Uses a small hard-coded DNS table.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#define PORT 5353
#define BUFSIZE 256

struct Entry { const char *host; const char *ip; };
static struct Entry table[] = {
    {"example.com", "93.184.216.34"},
    {"ssn.edu.in", "202.54.12.1"},
    {"localhost", "127.0.0.1"}
};

int main(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return 1;
    }
    printf("Simulated DNS server on UDP port %d\n", PORT);

    for (;;) {
        char query[BUFSIZE], reply[BUFSIZE] = "NXDOMAIN";
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        ssize_t n = recvfrom(fd, query, sizeof(query)-1, 0,
                             (struct sockaddr *)&client, &len);
        if (n < 0) { perror("recvfrom"); continue; }
        query[n] = '\0';
        for (int i = 0; query[i]; i++) query[i] = (char)tolower((unsigned char)query[i]);

        for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
            if (strcmp(query, table[i].host) == 0) {
                snprintf(reply, sizeof(reply), "%s", table[i].ip);
                break;
            }
        }
        sendto(fd, reply, strlen(reply), 0,
               (struct sockaddr *)&client, len);
        printf("Query: %s -> %s\n", query, reply);
    }
}
