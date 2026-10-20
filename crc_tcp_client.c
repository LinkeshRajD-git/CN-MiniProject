// Computes CRC using generator 10011 and sends frame over TCP.
// Set INJECT_ERROR to 0 to test an uncorrupted frame.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9090
#define BUF 4096
#define GEN "10011"
#define INJECT_ERROR 1

int main(void) {
    char data[512], bits[BUF], work[BUF], frame[BUF];
    printf("Enter text data: ");
    if (!fgets(data, sizeof(data), stdin)) return 1;
    data[strcspn(data, "\r\n")] = '\0';
    if (!data[0]) { puts("Empty input."); return 1; }

    size_t k = 0;
    for (size_t i = 0; data[i]; i++) {
        unsigned char ch = (unsigned char)data[i];
        for (int b = 7; b >= 0; b--)
            bits[k++] = ((ch >> b) & 1) ? '1' : '0';
    }
    bits[k] = '\0';
    strcpy(work, bits);
    size_t n = strlen(bits), g = strlen(GEN);
    for (size_t i = 0; i < g-1; i++) work[n+i] = '0';
    work[n+g-1] = '\0';
    for (size_t i = 0; i + g <= n+g-1; i++) {
        if (work[i] == '1') {
            for (size_t j = 0; j < g; j++)
                work[i+j] = (work[i+j] == GEN[j]) ? '0' : '1';
        }
    }
    memcpy(frame, bits, n);
    memcpy(frame+n, work+n, g-1);
    frame[n+g-1] = '\0';
    printf("Data bits: %s\nCRC remainder: %.*s\n",
           bits, (int)(g-1), work+n);
    printf("Frame before transmission: %s\n", frame);

#if INJECT_ERROR
    if (frame[0] == '0') frame[0] = '1'; else frame[0] = '0';
    puts("One bit was flipped intentionally.");
#endif

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (connect(fd, (struct sockaddr *)&a, sizeof(a)) < 0) {
        perror("connect"); close(fd); return 1;
    }
    if (send(fd, frame, strlen(frame), 0) < 0) perror("send");
    close(fd);
    return 0;
}
