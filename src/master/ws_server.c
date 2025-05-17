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

/* --------------- WS handler --------------------------------------- */
static int ws_connect(const struct mg_connection *conn, void **cbData)
{
    const char *id = mg_get_header(conn, "Sec-WebSocket-Protocol");
    (void)id;    /* unused for now */
    /* we could parse ?id= in URI instead */
    return 0;    /* accept */
}

static void ws_ready(struct mg_connection *conn, void *cbData)
{
    (void)cbData;
    printf("[MASTER] WS ready\n");
}

static int ws_data(struct mg_connection *conn, int flags,
                   char *data, size_t len, void *cbData)
{
    struct conn_ctx *ctx = (struct conn_ctx *)cbData;
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    if (!root) return 1;

    if (strcmp(ws_get_type(root), WS_HELLO) == 0) {
        const cJSON *pl = ws_get_payload(root);
        const cJSON *role = cJSON_GetObjectItemCaseSensitive(pl, "role");
        const cJSON *profile = cJSON_GetObjectItemCaseSensitive(pl, "profile");

        snprintf(ctx->node_id, sizeof(ctx->node_id), "%s",
                 cJSON_GetStringValue(cJSON_GetObjectItem(root, "node_id")));
        printf("[MASTER] HELLO from %s (profile: %s)\n",
               ctx->node_id, cJSON_IsString(profile) ? profile->valuestring : "?");

        /* --- send config blob back --- */
        const char *profname = cJSON_IsString(profile) ? profile->valuestring : "";
        const cJSON *prof = cJSON_GetObjectItemCaseSensitive(
                                cJSON_GetObjectItemCaseSensitive(
                                    cJSON_GetObjectItemCaseSensitive(root, ".."), /* placeholder */
                                    "profiles"), profname);
        char *cfg_json = cJSON_PrintUnformatted((cJSON*)prof);

        cJSON *payload = cJSON_Parse("{}");
        cJSON_AddRawToObject(payload, "profile", cfg_json);

        char *cfg_frame = ws_wrap_payload(
                              WS_CONFIG, ctx->node_id,
                              ws_next_seq(), payload);

        mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT,
                           cfg_frame, strlen(cfg_frame));
        free(cfg_frame);
        free(cfg_json);
    }
    cJSON_Delete(root);
    return 1;  /* keep connection open */
}

static void ws_close(const struct mg_connection *c, void *cbData)
{
    struct conn_ctx *ctx = (struct conn_ctx *)cbData;
    printf("[MASTER] WS closed (%s)\n", ctx ? ctx->node_id : "?");
    free(ctx);
}

static const struct mg_websocket_subprotocols dummy_sub[] = { {NULL, NULL} };

static const struct mg_websocket_handler ws_handler = {
    .connect_handler = ws_connect,
    .ready_handler   = ws_ready,
    .data_handler    = ws_data,
    .close_handler   = ws_close,
    .subprotocols    = dummy_sub
};

/* --------------- server entry ------------------------------------- */
int ws_server_start(const char *json_path, const char *port)
{
    const char *opts[] = { "listening_ports", port,
                           "error_log_file",  "error.log",
                           NULL };

    struct mg_context *ctx = mg_start(NULL, 0, opts);
    if (!ctx) { fprintf(stderr, "Civetweb start failed\n"); return -1; }

    mg_set_websocket_handler(ctx, "/ws", &ws_handler);

    printf("[MASTER] Civetweb WS running on :%s\n", port);
    return 0;   /* runs in background; ctx kept globally if needed */
}
