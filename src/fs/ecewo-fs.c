#include "ecewo-fs.h"
#include "ecewo.h" // Only for get_loop()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uv_fs_t fs_req;
    void *user_data;

    // Callbacks
    fs_read_callback_t read_callback;
    fs_write_callback_t write_callback;
    fs_stat_callback_t stat_callback;

    // Data
    char *data;
    size_t size;
    uv_stat_t stat;

    // Internal
    uv_file file;
    size_t file_size;
    char *path;
} fs_request_t;

static char *make_error_msg(int errcode)
{
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s: %s", uv_err_name(errcode), uv_strerror(errcode));
    return buf;
}

static void fs_request_cleanup(fs_request_t *req, bool free_data)
{
    if (!req)
        return;

    if (req->path)
        free(req->path);

    if (free_data && req->data)
        free(req->data);

    free(req);
}

static void read_close_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    uv_fs_req_cleanup(req);

    fs_req->read_callback(NULL, fs_req->data, fs_req->size, fs_req->user_data);

    // Cleanup but don't free data, user owns it now
    fs_request_cleanup(fs_req, false);
}

static void read_data_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);
        uv_fs_close(get_loop(), &fs_req->fs_req, fs_req->file, NULL);

        fs_req->read_callback(error, NULL, 0, fs_req->user_data);
        fs_request_cleanup(fs_req, true);
        return;
    }

    fs_req->size = (size_t)req->result;
    fs_req->data[fs_req->size] = '\0';

    uv_fs_req_cleanup(req);
    uv_fs_close(get_loop(), &fs_req->fs_req, fs_req->file, read_close_cb);
}

static void read_open_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);

        fs_req->read_callback(error, NULL, 0, fs_req->user_data);
        fs_request_cleanup(fs_req, false);
        return;
    }

    fs_req->file = (uv_file)req->result;
    uv_fs_req_cleanup(req);

    fs_req->data = malloc(fs_req->file_size + 1);
    if (!fs_req->data) {
        uv_fs_close(get_loop(), &fs_req->fs_req, fs_req->file, NULL);

        fs_req->read_callback("Memory allocation failed", NULL, 0, fs_req->user_data);
        fs_request_cleanup(fs_req, false);
        return;
    }

    uv_buf_t buf = uv_buf_init(fs_req->data, (unsigned int)fs_req->file_size);
    uv_fs_read(get_loop(), &fs_req->fs_req, fs_req->file, &buf, 1, 0, read_data_cb);
}

static void read_stat_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);

        fs_req->read_callback(error, NULL, 0, fs_req->user_data);
        fs_request_cleanup(fs_req, false);
        return;
    }

    fs_req->file_size = (size_t)req->statbuf.st_size;
    uv_fs_req_cleanup(req);

    uv_fs_open(get_loop(), &fs_req->fs_req, fs_req->path,
               UV_FS_O_RDONLY, 0, read_open_cb);
}

void fs_read_file(const char *path, fs_read_callback_t callback, void *user_data)
{
    if (!path || !callback) {
        fprintf(stderr, "fs_read_file: Invalid arguments\n");
        return;
    }

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req) {
        fprintf(stderr, "fs_read_file: Memory allocation failed\n");
        return;
    }

    fs_req->user_data = user_data;
    fs_req->read_callback = callback;
    fs_req->path = strdup(path);
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path) {
        free(fs_req);
        return;
    }

    int result = uv_fs_stat(get_loop(), &fs_req->fs_req, fs_req->path, read_stat_cb);
    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, NULL, 0, user_data);
        free(fs_req->path);
        free(fs_req);
    }
}

static void write_close_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;
    uv_fs_req_cleanup(req);

    fs_req->write_callback(NULL, fs_req->user_data);
    fs_request_cleanup(fs_req, true);
}

static void write_data_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);
        uv_fs_close(get_loop(), &fs_req->fs_req, fs_req->file, NULL);

        fs_req->write_callback(error, fs_req->user_data);
        fs_request_cleanup(fs_req, true);
        return;
    }

    fs_req->size = (size_t)req->result;
    uv_fs_req_cleanup(req);
    uv_fs_close(get_loop(), &fs_req->fs_req, fs_req->file, write_close_cb);
}

static void write_open_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);

        fs_req->write_callback(error, fs_req->user_data);
        fs_request_cleanup(fs_req, true);
        return;
    }

    fs_req->file = (uv_file)req->result;
    uv_fs_req_cleanup(req);

    uv_buf_t buf = uv_buf_init(fs_req->data, (unsigned int)fs_req->size);
    uv_fs_write(get_loop(), &fs_req->fs_req, fs_req->file, &buf, 1, 0, write_data_cb);
}

static void fs_write_internal(const char *path, const void *data, size_t size, fs_write_callback_t callback, void *user_data, int flags)
{
    if (!path || !data || !callback) {
        fprintf(stderr, "fs_write: Invalid arguments\n");
        return;
    }

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req)
        return;

    fs_req->user_data = user_data;
    fs_req->write_callback = callback;
    fs_req->path = strdup(path);
    fs_req->data = malloc(size);
    fs_req->size = size;
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path || !fs_req->data) {
        fs_request_cleanup(fs_req, true);
        return;
    }

    memcpy(fs_req->data, data, size);

    int result = uv_fs_open(get_loop(), &fs_req->fs_req, fs_req->path,
                            flags, 0644, write_open_cb);

    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, user_data);
        fs_request_cleanup(fs_req, true);
    }
}

void fs_write_file(const char *path, const void *data, size_t size, fs_write_callback_t callback, void *user_data)
{
    fs_write_internal(path, data, size, callback, user_data,
                      UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_TRUNC);
}

void fs_append_file(const char *path, const void *data, size_t size, fs_write_callback_t callback, void *user_data)
{
    fs_write_internal(path, data, size, callback, user_data,
                      UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_APPEND);
}

static void stat_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    if (req->result < 0) {
        char *error = make_error_msg((int)req->result);
        uv_fs_req_cleanup(req);

        fs_req->stat_callback(error, NULL, fs_req->user_data);
        fs_request_cleanup(fs_req, false);
        return;
    }

    fs_req->stat = req->statbuf;
    uv_fs_req_cleanup(req);

    // Success
    fs_req->stat_callback(NULL, &fs_req->stat, fs_req->user_data);
    fs_request_cleanup(fs_req, false);
}

void fs_stat(const char *path, fs_stat_callback_t callback, void *user_data)
{
    if (!path || !callback)
        return;

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req)
        return;

    fs_req->user_data = user_data;
    fs_req->stat_callback = callback;
    fs_req->path = strdup(path);
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path) {
        free(fs_req);
        return;
    }

    int result = uv_fs_stat(get_loop(), &fs_req->fs_req, fs_req->path, stat_cb);
    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, NULL, user_data);
        free(fs_req->path);
        free(fs_req);
    }
}

static void simple_op_cb(uv_fs_t *req)
{
    fs_request_t *fs_req = (fs_request_t *)req->data;

    char *error = NULL;
    if (req->result < 0)
        error = make_error_msg((int)req->result);

    uv_fs_req_cleanup(req);

    fs_req->write_callback(error, fs_req->user_data);
    fs_request_cleanup(fs_req, false);
}

typedef int (*uv_fs_op_t)(uv_loop_t *, uv_fs_t *, const char *, uv_fs_cb);
typedef int (*uv_fs_op_mode_t)(uv_loop_t *, uv_fs_t *, const char *, int, uv_fs_cb);

static void fs_simple_op(const char *path, fs_write_callback_t callback, void *user_data, uv_fs_op_t op_fn)
{
    if (!path || !callback)
        return;

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req)
        return;

    fs_req->user_data = user_data;
    fs_req->write_callback = callback;
    fs_req->path = strdup(path);
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path) {
        free(fs_req);
        return;
    }

    int result = op_fn(get_loop(), &fs_req->fs_req, fs_req->path, simple_op_cb);
    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, user_data);
        free(fs_req->path);
        free(fs_req);
    }
}

static void fs_simple_op_mode(const char *path, fs_write_callback_t callback, void *user_data, int mode, uv_fs_op_mode_t op_fn)
{
    if (!path || !callback)
        return;

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req)
        return;

    fs_req->user_data = user_data;
    fs_req->write_callback = callback;
    fs_req->path = strdup(path);
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path) {
        free(fs_req);
        return;
    }

    int result = op_fn(get_loop(), &fs_req->fs_req, fs_req->path, mode, simple_op_cb);
    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, user_data);
        free(fs_req->path);
        free(fs_req);
    }
}

void fs_unlink(const char *path, fs_write_callback_t callback, void *user_data)
{
    fs_simple_op(path, callback, user_data, uv_fs_unlink);
}

void fs_mkdir(const char *path, fs_write_callback_t callback, void *user_data)
{
    fs_simple_op_mode(path, callback, user_data, 0755, uv_fs_mkdir);
}

void fs_rmdir(const char *path, fs_write_callback_t callback, void *user_data)
{
    fs_simple_op(path, callback, user_data, uv_fs_rmdir);
}

void fs_rename(const char *old_path, const char *new_path, fs_write_callback_t callback, void *user_data)
{
    if (!old_path || !new_path || !callback)
        return;

    fs_request_t *fs_req = calloc(1, sizeof(fs_request_t));
    if (!fs_req)
        return;

    fs_req->user_data = user_data;
    fs_req->write_callback = callback;
    fs_req->path = strdup(new_path);
    fs_req->fs_req.data = fs_req;

    if (!fs_req->path) {
        free(fs_req);
        return;
    }

    char *old_copy = strdup(old_path);
    if (!old_copy) {
        fs_request_cleanup(fs_req, false);
        return;
    }

    int result = uv_fs_rename(get_loop(), &fs_req->fs_req, old_copy,
                              fs_req->path, simple_op_cb);
    free(old_copy);

    if (result < 0) {
        char *error = make_error_msg(result);
        callback(error, user_data);
        fs_request_cleanup(fs_req, false);
    }
}
