#include "ecewo-static.h"
#include "ecewo-fs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext)
        return "application/octet-stream";

    // Common web file types
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".json") == 0)
        return "application/json";
    if (strcmp(ext, ".xml") == 0)
        return "application/xml";

    // Images
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0)
        return "image/x-icon";
    if (strcmp(ext, ".webp") == 0)
        return "image/webp";

    // Fonts
    if (strcmp(ext, ".woff") == 0)
        return "font/woff";
    if (strcmp(ext, ".woff2") == 0)
        return "font/woff2";
    if (strcmp(ext, ".ttf") == 0)
        return "font/ttf";
    if (strcmp(ext, ".otf") == 0)
        return "font/otf";

    // Documents
    if (strcmp(ext, ".pdf") == 0)
        return "application/pdf";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";

    // Media
    if (strcmp(ext, ".mp4") == 0)
        return "video/mp4";
    if (strcmp(ext, ".webm") == 0)
        return "video/webm";
    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(ext, ".wav") == 0)
        return "audio/wav";

    return "application/octet-stream";
}

static bool is_safe_path(const char *path)
{
    if (strstr(path, "..") != NULL)
        return false;

    if (strstr(path, "//") != NULL)
        return false;

    return true;
}

typedef struct
{
    Res *res;
    char *mime_type;
} async_file_ctx_t;

typedef struct
{
    uv_write_t req;
    char *data;
} write_req_t;

static void on_write_complete(uv_write_t *req, int status)
{
    (void)status;
    write_req_t *wr = (write_req_t *)req;
    if (wr->data)
        free(wr->data);
    free(wr);
}

static void send_response_manual(uv_tcp_t *socket, int status_code, const char *content_type, const char *body, size_t body_len, bool keep_alive)
{
    if (!socket || uv_is_closing((uv_handle_t *)socket))
        return;

    const char *status_text = "OK";
    if (status_code == 404)
        status_text = "Not Found";
    else if (status_code == 403)
        status_text = "Forbidden";
    else if (status_code == 500)
        status_text = "Internal Server Error";

    size_t header_size = 512;
    size_t total_size = header_size + body_len;
    char *response = malloc(total_size);
    if (!response)
        return;

    int header_len = snprintf(response, header_size,
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Connection: %s\r\n"
                              "\r\n",
                              status_code, status_text,
                              content_type,
                              body_len,
                              keep_alive ? "keep-alive" : "close");

    if (body && body_len > 0)
        memcpy(response + header_len, body, body_len);

    write_req_t *wr = malloc(sizeof(write_req_t));
    if (!wr) {
        free(response);
        return;
    }

    wr->data = response;
    uv_buf_t buf = uv_buf_init(response, header_len + body_len);

    if (uv_write(&wr->req, (uv_stream_t *)socket, &buf, 1, on_write_complete) != 0) {
        free(response);
        free(wr);
    }
}

static void send_error_manual(uv_tcp_t *socket, int status_code, const char *message)
{
    send_response_manual(socket, status_code, "text/plain", message, strlen(message), false);
}

static void on_file_read(const char *error, const char *data, size_t size, void *user_data)
{
    async_file_ctx_t *ctx = (async_file_ctx_t *)user_data;

    ctx->res->replied = true;

    if (error) {
        send_text(ctx->res, 404, "File not found");
    } else {
        set_header(ctx->res, "Content-Type", ctx->mime_type);
        reply(ctx->res, 200, data, size);
    }

    if (data)
        free((void *)data);

    free(ctx->mime_type);
    free(ctx);
}

void send_file(Res *res, const char *filepath)
{
    if (!res || !filepath) {
        if (res)
            send_text(res, 500, "Invalid arguments");
        return;
    }

    if (!is_safe_path(filepath)) {
        send_text(res, 403, "Forbidden");
        return;
    }

    async_file_ctx_t *ctx = malloc(sizeof(async_file_ctx_t));
    if (!ctx) {
        send_text(res, 500, "Memory allocation failed");
        return;
    }

    ctx->res = res;
    ctx->mime_type = strdup(get_mime_type(filepath));

    if (!ctx->mime_type) {
        free(ctx);
        send_text(res, 500, "Memory allocation failed");
        return;
    }

    fs_read_file(filepath, on_file_read, ctx);
}

typedef struct
{
    char *mount_path;
    char *dir_path;
    size_t mount_len;
    Static options;
} static_ctx_t;

typedef struct
{
    static_ctx_t **items;
    int count;
    int capacity;
} static_ctx_array_t;

static static_ctx_array_t static_contexts = { 0 };

static void ensure_static_capacity(void)
{
    if (static_contexts.count >= static_contexts.capacity) {
        int new_capacity = static_contexts.capacity == 0 ? 4 : static_contexts.capacity * 2;
        static_ctx_t **new_items = realloc(static_contexts.items,
                                           new_capacity * sizeof(static_ctx_t *));
        if (!new_items) {
            fprintf(stderr, "serve_static: Memory allocation failed\n");
            return;
        }
        static_contexts.items = new_items;
        static_contexts.capacity = new_capacity;
    }
}

static void static_handler(Req *req, Res *res)
{
    const char *url_path = req->path;

    for (int i = 0; i < static_contexts.count; i++) {
        static_ctx_t *ctx = static_contexts.items[i];

        if (strncmp(url_path, ctx->mount_path, ctx->mount_len) != 0)
            continue;

        const char *rel_path = url_path + ctx->mount_len;
        if (*rel_path == '/')
            rel_path++;

        if (!ctx->options.dot_files && *rel_path == '.') {
            send_text(res, 403, "Forbidden");
            return;
        }

        // Check if this is a directory request (empty relative path or ends with /)
        bool is_dir = (*rel_path == '\0' || (strlen(rel_path) > 0 && rel_path[strlen(rel_path) - 1] == '/'));

        char filepath[1024];
        if (is_dir) {
            if (strlen(rel_path) == 0) {
                snprintf(filepath, sizeof(filepath), "%s/%s", ctx->dir_path, ctx->options.index_file);
            } else {
                snprintf(filepath, sizeof(filepath), "%s/%s%s", ctx->dir_path, rel_path, ctx->options.index_file);
            }
        } else {
            snprintf(filepath, sizeof(filepath), "%s/%s", ctx->dir_path, rel_path);
        }

        if (!is_safe_path(filepath)) {
            send_text(res, 403, "Forbidden");
            return;
        }

        send_file(res, filepath);
        return;
    }

    send_text(res, 404, "Not found");
}

void serve_static(const char *mount_path,
                  const char *dir_path,
                  const Static *options)
{
    if (!mount_path || !dir_path) {
        fprintf(stderr, "serve_static: Invalid arguments\n");
        return;
    }

    Static final_opts;

    final_opts.index_file = "index.html";
    final_opts.enable_etag = false;
    final_opts.enable_cache = false;
    final_opts.max_age = 3600;
    final_opts.dot_files = false;

    if (options) {
        if (options->index_file)
            final_opts.index_file = options->index_file;
        final_opts.enable_etag = options->enable_etag;
        final_opts.enable_cache = options->enable_cache;
        final_opts.max_age = options->max_age;
        final_opts.dot_files = options->dot_files;
    }

    ensure_static_capacity();
    if (static_contexts.count >= static_contexts.capacity) {
        fprintf(stderr, "serve_static: Failed to allocate memory\n");
        return;
    }

    static_ctx_t *ctx = malloc(sizeof(static_ctx_t));
    if (!ctx) {
        fprintf(stderr, "serve_static: Memory allocation failed\n");
        return;
    }

    ctx->mount_path = strdup(mount_path);
    ctx->dir_path = strdup(dir_path);
    ctx->mount_len = strlen(mount_path);
    ctx->options = final_opts;

    static_contexts.items[static_contexts.count++] = ctx;

    // Register exact mount path for directory access (e.g., "/" -> "/")
    get(mount_path, static_handler);

    // Register wildcard pattern for all sub-paths (e.g., "/*" or "/assets/*")
    char *route_pattern = malloc(strlen(mount_path) + 4);
    if (mount_path[strlen(mount_path) - 1] == '/') {
        sprintf(route_pattern, "%s*", mount_path);
    } else {
        sprintf(route_pattern, "%s/*", mount_path);
    }

    get(route_pattern, static_handler);
    free(route_pattern);
}

void static_cleanup(void)
{
    for (int i = 0; i < static_contexts.count; i++) {
        static_ctx_t *ctx = static_contexts.items[i];
        if (ctx) {
            free(ctx->mount_path);
            free(ctx->dir_path);
            free(ctx);
        }
    }

    if (static_contexts.items)
        free(static_contexts.items);

    static_contexts.items = NULL;
    static_contexts.count = 0;
    static_contexts.capacity = 0;
}
