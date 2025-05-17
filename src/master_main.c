#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "key_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* helper to replace {priv_key_path} placeholder */
static char *subst_priv_placeholder(const char *cmd, const char *path)
{
    const char *ph = "{priv_key_path}";
    const char *pos = strstr(cmd, ph);
    if (!pos) return strdup(cmd);

    size_t new_len = strlen(cmd) - strlen(ph) + strlen(path) + 1;
    char *out = malloc(new_len);
    if (!out) return NULL;

    size_t prefix = (size_t)(pos - cmd);
    memcpy(out, cmd, prefix);
    strcpy(out + prefix, path);
    strcpy(out + prefix + strlen(path), pos + strlen(ph));
    return out;
}

int main(int argc, char **argv)
{
    const char *json_path = (argc > 1) ? argv[1] : "fleet.json";

    if (config_load(json_path) != 0) {
        fprintf(stderr, "[MASTER] cannot load %s\n", json_path);
        return 1;
    }
    printf("[MASTER] config loaded: %s\n", json_path);

    /* fetch private key B64 from gs_master profile */
    const char *b64 = config_get_profile_str("gs_master", "private_key_b64");
    if (!b64) {
        fprintf(stderr, "[MASTER] missing private_key_b64 in profile gs_master\n");
        goto fail;
    }

    char *tmp_key = key_tempfile_from_b64(b64);
    if (!tmp_key) {
        fprintf(stderr, "[MASTER] failed to materialise temp key file\n");
        goto fail;
    }
    printf("[MASTER] temp key path: %s (will be unlinked later)\n", tmp_key);

    /* show first command with placeholder substituted */
    const char *raw_cmd = config_get_command("gs_master", "video_agg_rx");
    if (raw_cmd) {
        char *final_cmd = subst_priv_placeholder(raw_cmd, tmp_key);
        printf("[MASTER] sample command:\n  %s\n", final_cmd);
        free(final_cmd);
    } else {
        printf("[MASTER] video_agg_rx command not found\n");
    }

    /* simulate run then cleanup */
    unlink(tmp_key);      /* remove directory entry */
    free(tmp_key);
    config_unload();
    printf("[MASTER] demo done\n");
    return 0;

fail:
    config_unload();
    return 1;
}
