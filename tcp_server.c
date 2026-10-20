// Simple TCP echo server: receives one line and sends it back.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF 1024

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); return 1;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    printf("TCP server listening on port %d...\n", PORT);
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); close(server_fd); return 1; }

    char buf[BUF];
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) { perror("recv"); close(client_fd); close(server_fd); return 1; }
    buf[n] = '\0';
    printf("Client says: %s\n", buf);

    if (send(client_fd, buf, (size_t)n, 0) < 0) perror("send");
    close(client_fd);
    close(server_fd);
    return 0;
}
