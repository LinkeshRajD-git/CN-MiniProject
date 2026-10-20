// Minimal HTTP/1.0 client for a local HTTP server.
// Saves the response body to downloaded.bin and reports timing/throughput.
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define RESP_CAP (16 * 1024 * 1024)

static double elapsed(struct timespec a, struct timespec b) {
    return (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
}

int main(void) {
    char url[1024], host[256], path[768];
    printf("URL (example http://127.0.0.1:8000/index.html): ");
    if (!fgets(url, sizeof(url), stdin)) return 1;
    url[strcspn(url, "\r\n")] = '\0';

    if (strncmp(url, "http://", 7) != 0) {
        puts("Only http:// URLs are supported by this teaching client.");
        return 1;
    }
    char *p = url + 7, *slash = strchr(p, '/');
    char hostport[320];
    if (slash) {
        size_t hn = (size_t)(slash-p);
        if (hn == 0 || hn >= sizeof(hostport)) return 1;
        memcpy(hostport, p, hn); hostport[hn] = '\0';
        snprintf(path, sizeof(path), "%s", slash);
    } else {
        snprintf(hostport, sizeof(hostport), "%s", p);
        strcpy(path, "/");
    }
    char *colon = strrchr(hostport, ':');
    char port[16] = "80";
    if (colon) {
        *colon++ = '\0';
        snprintf(port, sizeof(port), "%s", colon);
    }
    snprintf(host, sizeof(host), "%s", hostport);

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    int ga = getaddrinfo(host, port, &hints, &res);
    if (ga != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ga)); return 1; }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { perror("socket"); freeaddrinfo(res); return 1; }

    struct timespec t0, t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect"); close(fd); freeaddrinfo(res); return 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    freeaddrinfo(res);

    char request[2048];
    int rq = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, host);
    if (rq < 0 || send(fd, request, (size_t)rq, 0) < 0) {
        perror("send"); close(fd); return 1;
    }

    char *response = malloc(RESP_CAP);
    if (!response) { perror("malloc"); close(fd); return 1; }
    size_t used = 0;
    while (used < RESP_CAP-1) {
        ssize_t n = recv(fd, response+used, RESP_CAP-1-used, 0);
        if (n == 0) break;
        if (n < 0) { perror("recv"); free(response); close(fd); return 1; }
        used += (size_t)n;
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    close(fd);
    response[used] = '\0';

    char *body = strstr(response, "\r\n\r\n");
    if (!body) { puts("Invalid HTTP response: headers not found."); free(response); return 1; }
    body += 4;
    size_t header_bytes = (size_t)(body-response);
    size_t body_bytes = used-header_bytes;

    FILE *out = fopen("downloaded.bin", "wb");
    if (!out) { perror("fopen"); free(response); return 1; }
    fwrite(body, 1, body_bytes, out);
    fclose(out);

    printf("HTTP status line: ");
    char *line_end = strstr(response, "\r\n");
    if (line_end) { *line_end = '\0'; printf("%s\n", response); }
    printf("TCP connection time: %.3f ms\n", elapsed(t0,t1)*1000.0);
    printf("Download/response time: %.3f ms\n", elapsed(t1,t2)*1000.0);
    printf("Body bytes saved: %zu\n", body_bytes);
    double sec = elapsed(t1,t2);
    if (sec > 0) printf("Body throughput: %.3f Mbps\n", body_bytes*8.0/sec/1e6);
    puts("Saved response body as downloaded.bin");
    free(response);
    return 0;
}
