#ifndef WFBMGR_CONFIG_H
#define WFBMGR_CONFIG_H

#include <stddef.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

int  config_load(const char *json_path);
void config_unload(void);

/* profile-level getters */
const char **config_get_init_script(const char *profile, size_t *len_out);
const char  *config_get_command    (const char *profile, const char *cmd_key);
const char  *config_get_template   (const char *profile, const char *tpl_key);
const cJSON *config_get_root_cjson(void);


/* generic: fetch an arbitrary string field from a profile (e.g. keys) */
const char  *config_get_profile_str(const char *profile, const char *field);

#ifdef __cplusplus
}
#endif
#endif
