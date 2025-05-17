#define _POSIX_C_SOURCE 200809L
#include "ws_server.h"
#include "config.h"
#include "ws_proto.h"
#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* optional per-connection scratch */
struct conn_ctx { char node_id[64]; };

/* ------------- callback signatures Civetweb expects ---------------- */

static int  cb_connect(const struct mg_connection *conn, void *cbdata);
static void cb_ready  (struct mg_connection *conn, void *cbdata);
static int  cb_data   (struct mg_connection *conn, int flags,
                       char *data, size_t len, void *cbdata);
static void cb_close  (struct mg_connection *conn, void *cbdata);

/* ------------------------------------------------------------------- */
static int cb_connect(const struct mg_connection *conn, void *cbdata)
{
    (void)conn; (void)cbdata;
    struct conn_ctx *ctx = calloc(1, sizeof(*ctx));
    mg_set_user_connection_data(conn, ctx);   /* store pointer */
    return 0;                                /* accept */
}

static void cb_ready(struct mg_connection *conn, void *cbdata)
{
    (void)cbdata;
    printf("[MASTER] WS ready\n");
}

static int cb_data(struct mg_connection *conn, int flags,
                   char *data, size_t len, void *cbdata)
{
    struct conn_ctx *ctx = mg_get_user_connection_data(conn);
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    if (!root) return 1;

    const char *type = ws_get_type(root);
    if (type && strcmp(type, WS_HELLO) == 0) {
        const char *node =
            cJSON_GetStringValue(cJSON_GetObjectItem(root, "node_id"));
        if (node) strncpy(ctx->node_id, node, sizeof(ctx->node_id) - 1);

        printf("[MASTER] HELLO from %s\n", ctx->node_id);

        /* send stub config frame */
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "note", "config TBD");

        char *frame = ws_wrap_payload(WS_CONFIG, ctx->node_id,
                                      ws_next_seq(), payload);

        mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT,
                           frame, strlen(frame));
        free(frame);
    }

    cJSON_Delete(root);
    return 1;  /* keep connection open */
}

static void cb_close(const struct mg_connection *conn, void *cbdata)
{
    struct conn_ctx *ctx = mg_get_user_connection_data(conn);
    printf("[MASTER] WS closed (%s)\n",
           ctx && ctx->node_id[0] ? ctx->node_id : "unknown");
    free(ctx);
}

/* ----------------------- server bootstrap -------------------------- */
int ws_server_start(const char *json_path, const char *port)
{
    (void)json_path;          /* config use comes later */

    const char *opts[] = { "listening_ports", port,
                           "error_log_file",  "error.log",
                           NULL };

    struct mg_context *ctx = mg_start(NULL, 0, opts);
    if (!ctx) { fprintf(stderr, "Civetweb start failed\n"); return -1; }

    mg_set_websocket_handler(ctx, "/ws",
                             cb_connect,
                             cb_ready,
                             cb_data,
                             cb_close,
                             NULL);

    printf("[MASTER] Civetweb WS listening on :%s\n", port);
    return 0;
}
