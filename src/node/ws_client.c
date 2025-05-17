#define _POSIX_C_SOURCE 200809L
#include "ws_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

/* ------------------------------------------------------------------ */

static int tcp_connect(const char *host, const char *port)
{
    struct addrinfo hints = { .ai_family = AF_UNSPEC,
                              .ai_socktype = SOCK_STREAM };
    struct addrinfo *res, *rp;
    if (getaddrinfo(host, port, &hints, &res) != 0) return -1;

    int fd = -1;
    for (rp = res; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(fd); fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

static int ws_handshake(int fd, const char *host, const char *path)
{
    /* Constant base-64 nonce (dummy). Valid per RFC if server accepts it. */
    const char *b64_nonce = "dGhpc2lzbXlub25jZQ==";

    dprintf(fd,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        path, host, b64_nonce);

    char resp[256];
    ssize_t n = read(fd, resp, sizeof(resp) - 1);
    if (n <= 0) return -1;
    resp[n] = '\0';
    return (strstr(resp, "101") != NULL) ? 0 : -1;
}

/* minimal unmasked TEXT frame (payload ≤125) */
static int ws_send_text(int fd, const char *txt)
{
    size_t len = strlen(txt);
    if (len > 125) return -1;
    unsigned char hdr[2] = { 0x81, (unsigned char)len }; /* FIN | TEXT, no mask */
    if (write(fd, hdr, 2) != 2) return -1;
    return (write(fd, txt, len) == (ssize_t)len) ? 0 : -1;
}

/* ------------------------------------------------------------------ */

int main(void)
{
    int fd = tcp_connect("192.168.2.10", "8080");
    if (fd < 0) { perror("connect"); return 1; }

    if (ws_handshake(fd, "192.168.2.10", "/ws") != 0) {
        fprintf(stderr, "handshake failed\n"); close(fd); return 1;
    }
    printf("[NODE] WS handshake OK\n");

    char *hello = ws_build_hello("gs-local", ws_next_seq(),
                                 "node", "gs_local", "0.1-dev");
    ws_send_text(fd, hello);
    free(hello);
    printf("[NODE] hello sent\n");

    /* Read one server frame (naïve) */
    unsigned char hdr[2];
    read(fd, hdr, 2);
    size_t len = hdr[1] & 0x7F;
    char *buf = malloc(len + 1);
    read(fd, buf, len); buf[len] = '\0';
    printf("[NODE] server frame: %s\n", buf);
    free(buf);

    close(fd);
    return 0;
}
