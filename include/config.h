#ifndef WFBMGR_CONFIG_H
#define WFBMGR_CONFIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Call once at program start. Returns 0 on success. */
int  config_load(const char *json_path);

/* Free all internal cJSON allocations. */
void config_unload(void);

/* ------------------ profile-level getters ------------------ */

/* Returns array of C strings (NULL-terminated). len_out can be NULL.   *
 *  Caller MUST NOT free the returned array nor the strings.            */
const char **config_get_init_script(const char *profile, size_t *len_out);

/* Returns single command string for profile/service or NULL.           */
const char *config_get_command(const char *profile,
                               const char *command_key);

/* Returns template string (e.g. "set_radio") or NULL.                  */
const char *config_get_template(const char *profile,
                                const char *template_key);

#ifdef __cplusplus
}
#endif
#endif /* WFBMGR_CONFIG_H */
