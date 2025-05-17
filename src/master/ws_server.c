#define _POSIX_C_SOURCE 200809L
#include "ws_server.h"
#include "ws_proto.h"
#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* very small helper: send an unmasked text frame (len ≤125)           */
static int ws_send_text(struct mg_connection *c, const char *txt)
{
    size_t len = strlen(txt);
    if (len > 125) return -1;                    /* keep prototype simple */

    unsigned char hdr[2] = { 0x81, (unsigned char)len };  /* FIN | TEXT */

    if (mg_write(c, hdr, 2) != 2) return -1;
    if (mg_write(c, txt, len) != (int)len) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* per-connection scratch                                             */
struct conn_ctx { char node_id[64]; };

/* Civetweb callback prototypes                                       */
static int  cb_connect(const struct mg_connection *c, void *cb);
static void cb_ready  (struct mg_connection *c, void *cb);
static int  cb_data   (struct mg_connection *c, int flags,
                       char *data, size_t len, void *cb);
static void cb_close  (const struct mg_connection *c, void *cb);

/* ------------------ connect ---------------------------------------- */
static int cb_connect(const struct mg_connection *c, void *cb)
{
    (void)cb;
    struct conn_ctx *ctx = calloc(1, sizeof(*ctx));
    mg_set_user_connection_data(c, ctx);
    return 0;                       /* accept */
}

/* ------------------ ready ------------------------------------------ */
static void cb_ready(struct mg_connection *c, void *cb)
{
    (void)c; (void)cb;
    printf("[MASTER] WS ready\n");
}

/* ------------------ data ------------------------------------------- */
static int cb_data(struct mg_connection *c, int flags,
                   char *data, size_t len, void *cb)
{
    (void)flags; (void)cb;
    struct conn_ctx *ctx = mg_get_user_connection_data(c);

    data[len] = '\0';
    cJSON *root = cJSON_Parse(data);
    if (!root) return 1;

    const char *type = ws_get_type(root);

    if (type && strcmp(type, WS_HELLO) == 0) {
        const char *nid =
            cJSON_GetStringValue(cJSON_GetObjectItem(root, "node_id"));
        if (nid) strncpy(ctx->node_id, nid, sizeof(ctx->node_id) - 1);

        printf("[MASTER] HELLO from %s\n", ctx->node_id);

        /* ---- stub config payload ---- */
        cJSON *pl = cJSON_CreateObject();
        cJSON_AddStringToObject(pl, "note", "config TBD");

        char *frame = ws_wrap_payload(WS_CONFIG,
                                      ctx->node_id,
                                      ws_next_seq(),
                                      pl);

        ws_send_text(c, frame);     /* send via raw mg_write() helper */
        free(frame);
    }

    cJSON_Delete(root);
    return 1;                       /* keep socket open */
}

/* ------------------ close ------------------------------------------ */
static void cb_close(const struct mg_connection *c, void *cb)
{
    (void)c; (void)cb;
    struct conn_ctx *ctx = mg_get_user_connection_data(c);
    printf("[MASTER] WS closed (%s)\n",
           ctx && ctx->node_id[0] ? ctx->node_id : "unknown");
    free(ctx);
}

/* ------------------ bootstrap -------------------------------------- */
int ws_server_start(const char *json_path, const char *port)
{
    (void)json_path;    /* future use: serve real profile slice */

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
                             NULL);

    printf("[MASTER] Civetweb WS listening on :%s\n", port);
    return 0;
}
