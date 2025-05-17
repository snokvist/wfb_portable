#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* internal helpers */

static cJSON *root = NULL;          /* whole document stays resident */

static const cJSON *find_profile(const char *name) {
    const cJSON *profiles = cJSON_GetObjectItemCaseSensitive(root, "profiles");
    if (!profiles || !cJSON_IsObject(profiles)) return NULL;
    return cJSON_GetObjectItemCaseSensitive(profiles, name);
}

static const cJSON *get_profile_field(const char *profile,
                                      const char *field) {
    const cJSON *p = find_profile(profile);
    return p ? cJSON_GetObjectItemCaseSensitive(p, field) : NULL;
}

/* ------------------------------------------------------------------ */
/* public API */

int config_load(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);

    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(fp); return -1; }

    fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[sz] = '\0';

    root = cJSON_Parse(buf);
    free(buf);

    return root ? 0 : -1;
}

void config_unload(void)
{
    if (root) { cJSON_Delete(root); root = NULL; }
}

/* returns NULL-terminated string array or NULL on error */
const char **config_get_init_script(const char *profile, size_t *len_out)
{
    const cJSON *arr = get_profile_field(profile, "init_script");
    if (!arr || !cJSON_IsArray(arr)) return NULL;

    size_t n = cJSON_GetArraySize(arr);
    if (len_out) *len_out = n;

    /* Cache pointer array in static storage so we build it only once.  *
     * In typical use profiles are loaded once at start-up.             */
    static const char **cache = NULL;
    static const char  *cached_profile = NULL;

    if (cache && cached_profile && strcmp(cached_profile, profile) == 0)
        return cache;

    cache = (const char **)calloc(n + 1, sizeof(char *));
    if (!cache) return NULL;

    for (size_t i = 0; i < n; ++i) {
        const cJSON *item = cJSON_GetArrayItem(arr, (int)i);
        cache[i] = cJSON_IsString(item) ? item->valuestring : NULL;
    }
    cache[n] = NULL;
    cached_profile = profile;
    return cache;
}

const char *config_get_profile_str(const char *profile, const char *field)
{
    const cJSON *val = get_profile_field(profile, field);
    return (val && cJSON_IsString(val)) ? val->valuestring : NULL;
}

const char *config_get_command(const char *profile, const char *cmd)
{
    const cJSON *cmds = get_profile_field(profile, "commands");
    if (!cmds || !cJSON_IsObject(cmds)) return NULL;
    const cJSON *val = cJSON_GetObjectItemCaseSensitive(cmds, cmd);
    return (val && cJSON_IsString(val)) ? val->valuestring : NULL;
}

const char *config_get_template(const char *profile, const char *tpl)
{
    const cJSON *tps = get_profile_field(profile, "live_cmd_templates");
    if (!tps || !cJSON_IsObject(tps)) return NULL;
    const cJSON *val = cJSON_GetObjectItemCaseSensitive(tps, tpl);
    return (val && cJSON_IsString(val)) ? val->valuestring : NULL;
}
