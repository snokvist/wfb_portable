#define _POSIX_C_SOURCE 200809L
#include "ws_server.h"
#include "ws_proto.h"
#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- compatibility layer ------------------------------------------ */
#ifndef WEBSOCKET_OPCODE_TEXT
#define WEBSOCKET_OPCODE_TEXT 1
#endif

/* Newer Civetweb provides mg_send_websocket_frame(), older versions
   expose mg_websocket_write().  Detect and bridge. */
#if !defined(HAVE_MG_WEBSOCKET_WRITE) && defined(mg_send_websocket_frame)
/* Header has only the new symbol — create a thin wrapper */
static int mg_websocket_write(struct mg_connection *c,
                              int opcode,
                              const char *data,
                              size_t len)
{
    return mg_send_websocket_frame(c, opcode, data, len);
}
#endif
/* ------------------------------------------------------------------- */

/* Per-connection scratch */
struct conn_ctx { char node_id[64]; };

/* Forward declarations */
static int  cb_connect(const struct mg_connection *, void *);
static void cb_ready  (struct mg_connection *, void *);
static int  cb_data   (struct mg_connection *, int, char *, size_t, void *);
static void cb_close  (const struct mg_connection *, void *);

/* ------------------ connect ---------------------------------------- */
static int cb_connect(const struct mg_connection *c, void *cbdata)
{
    (void)cbdata;
    struct conn_ctx *ctx = calloc(1, sizeof(*ctx));
    mg_set_user_connection_data(c, ctx);
    return 0;           /* accept */
}

/* ------------------ ready ------------------------------------------ */
static void cb_ready(struct mg_connection *c, void *cbdata)
{
    (void)c; (void)cbdata;
    printf("[MASTER] WS ready\n");
}

/* ------------------ data ------------------------------------------- */
static int cb_data(struct mg_connection *c, int flags,
                   char *data, size_t len, void *cbdata)
{
    (void)flags; (void)cbdata;
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

        /* ── stub config payload ── */
        cJSON *pl = cJSON_CreateObject();
        cJSON_AddStringToObject(pl, "note", "config TBD");

        char *frame = ws_wrap_payload(WS_CONFIG,
                                      ctx->node_id,
                                      ws_next_seq(),
                                      pl);

        mg_websocket_write(c, WEBSOCKET_OPCODE_TEXT,
                           frame, strlen(frame));
        free(frame);
    }

    cJSON_Delete(root);
    return 1;                       /* keep socket open */
}

/* ------------------ close ------------------------------------------ */
static void cb_close(const struct mg_connection *c, void *cbdata)
{
    (void)c; (void)cbdata;
    struct conn_ctx *ctx = mg_get_user_connection_data(c);
    printf("[MASTER] WS closed (%s)\n",
           ctx && ctx->node_id[0] ? ctx->node_id : "unknown");
    free(ctx);
}

/* ------------------ bootstrap -------------------------------------- */
int ws_server_start(const char *json_path, const char *port)
{
    (void)json_path;      /* reserved for later */

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
                             NULL);             /* cbdata */

    printf("[MASTER] Civetweb WS listening on :%s\n", port);
    return 0;
}
