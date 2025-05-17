#define _POSIX_C_SOURCE 200809L
#include "ws_server.h"
#include "ws_proto.h"
#include "config.h"
#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* helper: send unmasked text frame (payload ≤125 B for simplicity)    */
static int ws_send_text(struct mg_connection *c, const char *txt)
{
    size_t len = strlen(txt);
    if (len > 125) return -1;                 /* size guard */

    unsigned char hdr[2] = { 0x81, (unsigned char)len }; /* FIN|TEXT */
    if (mg_write(c, hdr, 2) != 2)          return -1;
    if (mg_write(c, txt, len) != (int)len) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* per-connection scratch structure                                    */
struct ctx {
    char node_id[64];
    char profile[64];
};

/* forward declarations (match Civetweb prototypes) */
static int  cb_connect(const struct mg_connection *c, void *cb);
static void cb_ready  (struct mg_connection *c, void *cb);
static int  cb_data   (struct mg_connection *c, int flags,
                       char *data, size_t len, void *cb);
static void cb_close  (const struct mg_connection *c, void *cb);

/* ------------------------------------------------------------------ */
static int cb_connect(const struct mg_connection *c, void *cb)
{
    (void)cb;
    struct ctx *x = calloc(1, sizeof *x);
    mg_set_user_connection_data(c, x);
    return 0;                               /* accept */
}

static void cb_ready(struct mg_connection *c, void *cb)
{
    (void)c; (void)cb;
    printf("[MASTER] WS ready\n");
}

extern cJSON *g_root_cfg;                   /* from master_main.c */

static int cb_data(struct mg_connection *c, int flags,
                   char *data, size_t len, void *cb)
{
    (void)flags; (void)cb;
    struct ctx *x = mg_get_user_connection_data(c);

    data[len] = '\0';
    cJSON *root = cJSON_Parse(data);
    if (!root) return 1;

    if (strcmp(ws_get_type(root), WS_HELLO) == 0) {
        /* cache node_id & requested profile */
        const cJSON *pl = ws_get_payload(root);
        strncpy(x->node_id,
                cJSON_GetStringValue(cJSON_GetObjectItem(root, "node_id")),
                sizeof x->node_id - 1);
        strncpy(x->profile,
                cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(pl, "profile")),
                sizeof x->profile - 1);

        printf("[MASTER] HELLO from %s (profile %s)\n",
               x->node_id, x->profile);

        /* ---- build profile slice ---- */
        const cJSON *profiles =
            cJSON_GetObjectItemCaseSensitive(g_root_cfg, "profiles");
        const cJSON *prof =
            cJSON_GetObjectItemCaseSensitive(profiles, x->profile);

        if (!prof) {
            fprintf(stderr, "[MASTER] unknown profile '%s'\n", x->profile);
            cJSON_Delete(root);
            return 0;
        }

        cJSON *payload = cJSON_Duplicate(prof, 1);   /* deep copy */

        char *frame = ws_wrap_payload(WS_CONFIG,
                                      x->node_id,
                                      ws_next_seq(),
                                      payload);

        ws_send_text(c, frame);      /* send */
        free(frame);
    }

    cJSON_Delete(root);
    return 1;                         /* keep socket open */
}

static void cb_close(const struct mg_connection *c, void *cb)
{
    (void)c; (void)cb;
    struct ctx *x = mg_get_user_connection_data(c);
    printf("[MASTER] WS closed (%s)\n",
           x && x->node_id[0] ? x->node_id : "unknown");
    free(x);
}

/* ------------------------------------------------------------------ */
int ws_server_start(const char *json_path, const char *port)
{
    (void)json_path;  /* already loaded into g_root_cfg */

    const char *opts[] = {
        "listening_ports", port,
        "error_log_file",  "error.log",
        NULL };

    struct mg_context *ctx = mg_start(NULL, 0, opts);
    if (!ctx) {
        fprintf(stderr, "Civetweb start failed\n");
        return -1;
    }

    mg_set_websocket_handler(ctx, "/ws",
                             cb_connect,
                             cb_ready,
                             cb_data,
                             cb_close,
                             NULL);               /* cbdata */

    printf("[MASTER] Civetweb WS listening on :%s\n", port);
    return 0;
}
