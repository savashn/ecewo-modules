#ifndef ECEWO_FS_H
#define ECEWO_FS_H

#include <stddef.h>
#include "uv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*fs_read_callback_t)(const char *error, const char *data, size_t size, void *user_data);
typedef void (*fs_write_callback_t)(const char *error, void *user_data);
typedef void (*fs_stat_callback_t)(const char *error, const uv_stat_t *stat, void *user_data);

void fs_read_file(const char *path, fs_read_callback_t callback, void *user_data);
void fs_write_file(const char *path, const void *data, size_t size, 
                   fs_write_callback_t callback, void *user_data);
void fs_append_file(const char *path, const void *data, size_t size,
                    fs_write_callback_t callback, void *user_data);
void fs_stat(const char *path, fs_stat_callback_t callback, void *user_data);
void fs_unlink(const char *path, fs_write_callback_t callback, void *user_data);
void fs_rename(const char *old_path, const char *new_path,
               fs_write_callback_t callback, void *user_data);
void fs_mkdir(const char *path, fs_write_callback_t callback, void *user_data);
void fs_rmdir(const char *path, fs_write_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
