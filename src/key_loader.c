#define _POSIX_C_SOURCE 200809L
#include "key_loader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* ---------- tiny Base-64 decode helper (URL-unsafe, RFC 4648) -------- */

static int b64_table[256];

static void init_tbl(void)
{
    static int done = 0;
    if (done) return;

    for (int i = 0; i < 256; ++i) b64_table[i] = -1;
    for (int i = 'A'; i <= 'Z'; ++i) b64_table[i] = i - 'A';
    for (int i = 'a'; i <= 'z'; ++i) b64_table[i] = i - 'a' + 26;
    for (int i = '0'; i <= '9'; ++i) b64_table[i] = i - '0' + 52;
    b64_table[(unsigned)'+'] = 62;
    b64_table[(unsigned)'/'] = 63;
    done = 1;
}

static unsigned char *b64_decode(const char *in, size_t *out_len)
{
    init_tbl();
    size_t len = strlen(in);
    size_t pad = 0;
    while (len && in[len - 1] == '=') { --len; ++pad; }

    size_t blen = (len * 3) / 4;
    unsigned char *out = (unsigned char *)malloc(blen);
    if (!out) return NULL;

    size_t o = 0;
    for (size_t i = 0; i < len; i += 4) {
        int v[4];
        for (int j = 0; j < 4; ++j)
            v[j] = b64_table[(unsigned)in[i + j]];

        out[o++] = (v[0] << 2) | (v[1] >> 4);
        if (i + 2 < len)
            out[o++] = (v[1] << 4) | (v[2] >> 2);
        if (i + 3 < len)
            out[o++] = (v[2] << 6) | v[3];
    }
    if (out_len) *out_len = o;
    return out;
}

/* -------------------------------------------------------------------- */

char *key_tempfile_from_b64(const char *b64)
{
    size_t key_len = 0;
    unsigned char *key_raw = b64_decode(b64, &key_len);
    if (!key_raw) return NULL;

    char tmpl[] = "/dev/shm/wfbkeyXXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) { free(key_raw); return NULL; }

    /* chmod 0600 and write bytes */
    fchmod(fd, S_IRUSR | S_IWUSR);
    ssize_t wr = write(fd, key_raw, key_len);
    free(key_raw);
    if (wr < 0 || (size_t)wr != key_len) { close(fd); unlink(tmpl); return NULL; }

    close(fd);

    /* caller freed */
    return strdup(tmpl);   /* strdup ok: small path */
}
