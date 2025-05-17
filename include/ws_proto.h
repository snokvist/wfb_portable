#ifndef WFBMGR_WS_PROTO_H
#define WFBMGR_WS_PROTO_H

#include "cJSON.h"
#include <stdint.h>

/* ---- Message type symbols ----------------------------------------- */
#define WS_HELLO      "hello"
#define WS_CONFIG     "config"
#define WS_HEARTBEAT  "heartbeat"
#define WS_LOG        "log"
#define WS_COMMAND    "command"
#define WS_CMDACK     "cmdAck"
#define WS_ALERT      "alert"

/* ---- Builder helpers (char* returned via cJSON_PrintUnformatted) --- */
/* caller must free() the returned string */

char *ws_build_hello(const char *node_id, uint32_t seq,
                     const char *role, const char *profile,
                     const char *version);

char *ws_build_cmd_ack(const char *node_id, uint32_t seq, int ok,
                       const char *msg);

/* generic payload wrapper: takes ownership of cJSON* payload */
char *ws_wrap_payload(const char *type, const char *node_id,
                      uint32_t seq, cJSON *payload);

/* ---- Parse helpers ------------------------------------------------- */
const char *ws_get_type(const cJSON *root);
uint32_t    ws_get_seq (const cJSON *root);
const cJSON*ws_get_payload(const cJSON *root);

/* ---- tiny monotonic seq generator ----------------------------------*/
uint32_t ws_next_seq(void);

#endif /* WFBMGR_WS_PROTO_H */
