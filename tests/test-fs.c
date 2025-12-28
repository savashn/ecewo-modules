#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-fs.h"
#include "tester.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// HANDLERS
// ============================================================================

static void on_read_complete(const char *error, const char *data, size_t size, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 404, error);
        return;
    }
    
    set_header(res, "Content-Type", "text/plain");
    reply(res, 200, data, size);
    
    if (data)
        free((void *)data);
}

static void on_write_complete(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_text(res, 201, "File written");
}

static void on_stat_complete(const char *error, const uv_stat_t *stat, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 404, error);
        return;
    }
    
    char *response = arena_sprintf(res->arena, "size:%lld", 
                                   (long long)stat->st_size);
    send_text(res, 200, response);
}

void handler_fs_read(Req *req, Res *res)
{
    const char *filename = get_query(req, "file");
    if (!filename)
    {
        send_text(res, 400, "Missing file parameter");
        return;
    }
    
    char *filepath = arena_sprintf(req->arena, "test_files/%s", filename);
    fs_read_file(filepath, on_read_complete, res);
}

void handler_fs_write(Req *req, Res *res)
{
    const char *filename = get_query(req, "file");
    if (!filename || !req->body)
    {
        send_text(res, 400, "Missing file or body");
        return;
    }
    
    char *filepath = arena_sprintf(req->arena, "test_files/%s", filename);
    fs_write_file(filepath, req->body, req->body_len, on_write_complete, res);
}

void handler_fs_stat(Req *req, Res *res)
{
    const char *filename = get_query(req, "file");
    if (!filename)
    {
        send_text(res, 400, "Missing file parameter");
        return;
    }
    
    char *filepath = arena_sprintf(req->arena, "test_files/%s", filename);
    fs_stat(filepath, on_stat_complete, res);
}

// ============================================================================
// TESTS
// ============================================================================

int test_fs_read_existing_file(void)
{
    uv_fs_t req;
    const char *content = "Hello from test file";
    
    uv_file file = uv_fs_open(NULL, &req, "test_files/test.txt", 
                              UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_TRUNC,
                              0644, NULL);
    uv_fs_req_cleanup(&req);
    
    if (file >= 0) {
        uv_buf_t buf = uv_buf_init((char*)content, strlen(content));
        uv_fs_write(NULL, &req, file, &buf, 1, -1, NULL);
        uv_fs_req_cleanup(&req);
        
        uv_fs_close(NULL, &req, file, NULL);
        uv_fs_req_cleanup(&req);
    }
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/fs/read?file=test.txt",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Hello from test file", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_fs_read_nonexistent_file(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/fs/read?file=nonexistent.txt",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(404, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

int test_fs_write_file(void)
{
    MockHeaders headers[] = {
        {"Content-Type", "text/plain"}
    };
    
    MockParams params = {
        .method = MOCK_POST,
        .path = "/fs/write?file=output.txt",
        .body = "Test content",
        .headers = headers,
        .header_count = 1
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(201, res.status_code);
    ASSERT_EQ_STR("File written", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_fs_stat_file(void)
{
    uv_fs_t req;
    const char *content = "12345";
    
    uv_file file = uv_fs_open(NULL, &req, "test_files/stat_test.txt", 
                              UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_TRUNC,
                              0644, NULL);
    uv_fs_req_cleanup(&req);
    
    if (file >= 0) {
        uv_buf_t buf = uv_buf_init((char*)content, strlen(content));
        uv_fs_write(NULL, &req, file, &buf, 1, -1, NULL);
        uv_fs_req_cleanup(&req);
        
        uv_fs_close(NULL, &req, file, NULL);
        uv_fs_req_cleanup(&req);
    }
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/fs/stat?file=stat_test.txt",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_NOT_NULL(strstr(res.body, "size:"));
    
    free_request(&res);
    RETURN_OK();
}

int test_fs_missing_parameter(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/fs/read",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(400, res.status_code);
    ASSERT_EQ_STR("Missing file parameter", res.body);
    
    free_request(&res);
    RETURN_OK();
}

// ============================================================================
// SETUP
// ============================================================================

void setup_fs_routes(void)
{
    uv_fs_t req;
    
    int r = uv_fs_mkdir(NULL, &req, "test_files", 0755, NULL);
    uv_fs_req_cleanup(&req);
    
    if (r != 0 && r != UV_EEXIST) {
        fprintf(stderr, "Failed to create test_files: %s\n", uv_strerror(r));
    }
    
    get("/fs/read", handler_fs_read);
    post("/fs/write", handler_fs_write);
    get("/fs/stat", handler_fs_stat);
}

void cleanup_fs(void)
{
    uv_fs_t req;
    
    uv_fs_scandir(NULL, &req, "test_files", 0, NULL);
    
    uv_dirent_t dent;
    while (uv_fs_scandir_next(&req, &dent) != UV_EOF) {
        char path[256];
        snprintf(path, sizeof(path), "test_files/%s", dent.name);
        
        uv_fs_t unlink_req;
        uv_fs_unlink(NULL, &unlink_req, path, NULL);
        uv_fs_req_cleanup(&unlink_req);
    }
    uv_fs_req_cleanup(&req);
    
    uv_fs_rmdir(NULL, &req, "test_files", NULL);
    uv_fs_req_cleanup(&req);
}
