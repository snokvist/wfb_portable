#ifndef WFBMGR_WS_SERVER_H
#define WFBMGR_WS_SERVER_H

int ws_server_start(const char *json_path,
                    const char *listen_port);  /* returns 0 OK */

#endif
