// Receives a text frame and CRC remainder over TCP, then verifies it.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9090
#define BUF 4096
#define GEN "10011"  // x^4 + x + 1

static int crc_bad(const char *frame) {
    char work[BUF];
    size_t n = strlen(frame), g = strlen(GEN);
    if (n >= sizeof(work) || n < g) return 1;
    strcpy(work, frame);
    for (size_t i = 0; i + g <= n; i++) {
        if (work[i] == '1') {
            for (size_t j = 0; j < g; j++)
                work[i+j] = (work[i+j] == GEN[j]) ? '0' : '1';
        }
    }
    for (size_t i = n - (g - 1); i < n; i++)
        if (work[i] != '0') return 1;
    return 0;
}

int main(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_port = htons(PORT);
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0 ||
        listen(s, 1) < 0) { perror("bind/listen"); close(s); return 1; }
    printf("CRC receiver listening on TCP %d, generator %s\n", PORT, GEN);
    int c = accept(s, NULL, NULL);
    if (c < 0) { perror("accept"); close(s); return 1; }
    char frame[BUF];
    ssize_t n = recv(c, frame, sizeof(frame)-1, 0);
    if (n < 0) { perror("recv"); close(c); close(s); return 1; }
    frame[n] = '\0';
    printf("Received frame: %s\n", frame);
    puts(crc_bad(frame) ? "CRC: ERROR DETECTED" : "CRC: frame appears error-free");
    close(c); close(s);
    return 0;
}
