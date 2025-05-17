#define _POSIX_C_SOURCE 200809L
#include "ws_server.h"
#include "config.h"
#include "ws_proto.h"
#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct conn_ctx {
    char node_id[64];
};

/* ---------- callbacks --------------------------------------------- */
static int cb_connect(const struct mg_connection *conn, void **cbData)
{
    /* accept all; create per-connection ctx */
    struct conn_ctx *c = calloc(1, sizeof(*c));
    *cbData = c;
    return 0;
}

static void cb_ready(struct mg_connection *conn, void *cbData)
{
    (void)conn; (void)cbData;
    printf("[MASTER] WS ready\n");
}

static int cb_data(struct mg_connection *conn, int flags,
                   char *data, size_t len, void *cbData)
{
    struct conn_ctx *ctx = (struct conn_ctx *)cbData;
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    if (!root) return 1;

    const char *type = ws_get_type(root);
    if (type && strcmp(type, WS_HELLO) == 0) {
        strncpy(ctx->node_id,
                cJSON_GetStringValue(cJSON_GetObjectItem(root, "node_id")),
                sizeof(ctx->node_id) - 1);

        const cJSON *pl = ws_get_payload(root);
        const char  *profile =
            cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(pl, "profile"));

        printf("[MASTER] HELLO from %s (profile %s)\n",
               ctx->node_id, profile ? profile : "?");

        /* ---- build config frame: just echo profile slice for demo ---- */
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "note", "config TBD");

        char *frame = ws_wrap_payload(
                          WS_CONFIG, ctx->node_id,
                          ws_next_seq(), payload);

        mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, frame, strlen(frame));
        free(frame);
    }
    cJSON_Delete(root);
    return 1;          /* keep connection open */
}

static void cb_close(const struct mg_connection *conn, void *cbData)
{
    struct conn_ctx *ctx = (struct conn_ctx *)cbData;
    printf("[MASTER] WS closed (%s)\n",
           ctx && ctx->node_id[0] ? ctx->node_id : "unknown");
    free(ctx);
}

/* ---------- server entry ------------------------------------------- */
int ws_server_start(const char *json_path, const char *port)
{
    (void)json_path;   /* will be used later for dynamic config frames */

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
                             NULL);         /* cbData argument to all cb */

    printf("[MASTER] Civetweb WS running on :%s\n", port);
    return 0;
}
