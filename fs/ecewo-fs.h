#ifndef ECEWO_FS_H
#define ECEWO_FS_H

#include <stddef.h>
#include "ecewo.h"
#include "uv.h"

// Forward declarations
typedef struct Req Req;
typedef struct Res Res;
typedef struct FSRequest FSRequest;

// Simple callback - error NULL means success
typedef void (*fs_callback_t)(FSRequest *req, const char *error);

// File operation request
struct FSRequest
{
    uv_fs_t fs_req;
    void *context; // User context (Req or Res)
    fs_callback_t callback;

    // Result data (allocated in arena)
    char *data;     // File content (for read)
    size_t size;    // Data size
    uv_stat_t stat; // File stats
};

// Read entire file - callback gets req->data and req->size
void fs_read_file(void *context, const char *path, fs_callback_t callback);

// Write entire file (creates or truncates)
void fs_write_file(void *context, const char *path,
                   const void *data, size_t size, fs_callback_t callback);

// Append to file
void fs_append_file(void *context, const char *path,
                    const void *data, size_t size, fs_callback_t callback);

// Get file stats - result in req->stat
void fs_stat(void *context, const char *path, fs_callback_t callback);

// Delete file
void fs_unlink(void *context, const char *path, fs_callback_t callback);

// Rename/move file
void fs_rename(void *context, const char *old_path, const char *new_path,
               fs_callback_t callback);

// Create directory
void fs_mkdir(void *context, const char *path, fs_callback_t callback);

// Remove directory
void fs_rmdir(void *context, const char *path, fs_callback_t callback);

#endif
