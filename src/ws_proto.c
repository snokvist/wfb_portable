#define _POSIX_C_SOURCE 200809L
#include "ws_proto.h"
#include <time.h>
#include <stdlib.h>

/* simple monotonically increasing counter */
uint32_t ws_next_seq(void)
{
    static uint32_t seq = 0;
    return ++seq;
}

static cJSON *base_frame(const char *type, const char *node_id, uint32_t seq)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "seq", seq);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));
    return root;
}

/* generic wrapper */
char *ws_wrap_payload(const char *type, const char *node_id,
                      uint32_t seq, cJSON *payload)
{
    cJSON *root = base_frame(type, node_id, seq);
    cJSON_AddItemToObject(root, "payload", payload);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

/* specialised builders */
char *ws_build_hello(const char *node_id, uint32_t seq,
                     const char *role, const char *profile,
                     const char *version)
{
    cJSON *pl = cJSON_CreateObject();
    cJSON_AddStringToObject(pl, "role", role);
    cJSON_AddStringToObject(pl, "profile", profile);
    cJSON_AddStringToObject(pl, "version", version);
    return ws_wrap_payload(WS_HELLO, node_id, seq, pl);
}

char *ws_build_cmd_ack(const char *node_id, uint32_t seq, int ok,
                       const char *msg)
{
    cJSON *pl = cJSON_CreateObject();
    cJSON_AddBoolToObject(pl, "ok", ok);
    cJSON_AddStringToObject(pl, "msg", msg ? msg : "");
    return ws_wrap_payload(WS_CMDACK, node_id, seq, pl);
}

/* parse helpers */
const char *ws_get_type(const cJSON *root)
{
    const cJSON *t = cJSON_GetObjectItemCaseSensitive(root, "type");
    return cJSON_IsString(t) ? t->valuestring : NULL;
}
uint32_t ws_get_seq(const cJSON *root)
{
    const cJSON *s = cJSON_GetObjectItemCaseSensitive(root, "seq");
    return cJSON_IsNumber(s) ? (uint32_t)s->valuedouble : 0;
}
const cJSON *ws_get_payload(const cJSON *root)
{
    return cJSON_GetObjectItemCaseSensitive(root, "payload");
}
