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
    const char *b64_nonce = "dGhpc2lzbXlub25jZQ==";

    dprintf(fd,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        path, host, b64_nonce);

    /* read until double-CRLF or 4 kB max */
    char buf[4096];
    size_t off = 0;
    while (off < sizeof(buf) - 1) {
        ssize_t n = read(fd, buf + off, sizeof(buf) - 1 - off);
        if (n <= 0) return -1;
        off += (size_t)n;
        buf[off] = '\0';
        if (strstr(buf, "\r\n\r\n"))
            break;                       /* end of headers */
    }
    if (strstr(buf, " 101 ") != NULL)
        return 0;

    /* Debug dump if handshake failed */
    fprintf(stderr, "\n--- Handshake response ---\n%.*s\n--------------------------\n",
            (int)off, buf);
    return -1;
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

static void apply_profile(const cJSON *prof)
{
    const cJSON *arr = cJSON_GetObjectItem(prof,"init_script");
    if(cJSON_IsArray(arr)){
        cJSON *it=NULL;
        cJSON_ArrayForEach(it,arr){
            if(cJSON_IsString(it)){
                system(it->valuestring);      /* simple */
            }
        }
    }
    const cJSON *cmds=cJSON_GetObjectItem(prof,"commands");
    if(cJSON_IsObject(cmds)){
        cJSON *it=NULL;
        cJSON_ArrayForEach(it,cmds){
            if(cJSON_IsString(it))
                proc_spawn_and_watch(it->valuestring); /* one per svc */
        }
    }
}



/* ------------------------------------------------------------------ */

int main(int argc,char**argv){
    const char*ip=(argc>1)?argv[1]:"127.0.0.1";
    const char*pt=(argc>2)?argv[2]:"8080";

    int fd=tcp_connect(ip,pt); if(fd<0){perror("conn");return1;}
    if(ws_handshake(fd,ip,"/ws")){fprintf(stderr,"HS fail\n");return1;}

    char*hello=ws_build_hello("gs-local",ws_next_seq(),"node","gs_local","0.1");
    ws_send_text(fd,hello); free(hello);

    /* wait for one config frame */
    unsigned char h[2]; read(fd,h,2);
    size_t len=h[1]&0x7F; char*buf=malloc(len+1); read(fd,buf,len);buf[len]=0;

    cJSON *root=cJSON_Parse(buf); free(buf);
    const cJSON*pl=ws_get_payload(root);
    apply_profile(pl);                   /* launch processes */
    cJSON_Delete(root);

    pause();                             /* keep node alive */
    return 0;
}
