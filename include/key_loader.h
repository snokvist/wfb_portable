#ifndef WFBMGR_KEY_LOADER_H
#define WFBMGR_KEY_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Decode Base-64 into a secure temp file.                               *
 * Returns malloc()-ed pathname (caller frees).                          *
 * The file is chmod 0600. On error returns NULL.                        */
char *key_tempfile_from_b64(const char *b64);

#ifdef __cplusplus
}
#endif
#endif /* WFBMGR_KEY_LOADER_H */
