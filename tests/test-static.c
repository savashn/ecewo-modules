#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-static.h"
#include "tester.h"
#include "uv.h"
#include <string.h>
#include <stdio.h>

int test_static_serve_html(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/index.html",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_NOT_NULL(strstr(res.body, "<html>"));
    
    free_request(&res);
    RETURN_OK();
}

int test_static_serve_index(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

int test_static_not_found(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/nonexistent.html",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(404, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

int test_static_dotfile_blocked(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/.env",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(403, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

int test_static_path_traversal_blocked(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/../../../etc/passwd",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_TRUE(res.status_code == 403 || res.status_code == 404);
    
    free_request(&res);
    RETURN_OK();
}

void setup_static_routes(void)
{
    uv_fs_t req;
    
    int r = uv_fs_mkdir(NULL, &req, "test_public", 0755, NULL);
    uv_fs_req_cleanup(&req);
    
    if (r != 0 && r != UV_EEXIST)
    {
        fprintf(stderr, "Failed to create test_public: %s\n", uv_strerror(r));
    }
    
    const char *index_content = "<html><body>Hello</body></html>";
    uv_file file = uv_fs_open(NULL, &req, "test_public/index.html",
                              UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_TRUNC,
                              0644, NULL);
    uv_fs_req_cleanup(&req);
    
    if (file >= 0)
    {
        uv_buf_t buf = uv_buf_init((char*)index_content, strlen(index_content));
        uv_fs_write(NULL, &req, file, &buf, 1, -1, NULL);
        uv_fs_req_cleanup(&req);
        
        uv_fs_close(NULL, &req, file, NULL);
        uv_fs_req_cleanup(&req);
    }
    
    const char *env_content = "SECRET=value";
    file = uv_fs_open(NULL, &req, "test_public/.env",
                      UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_TRUNC,
                      0644, NULL);
    uv_fs_req_cleanup(&req);
    
    if (file >= 0)
    {
        uv_buf_t buf = uv_buf_init((char*)env_content, strlen(env_content));
        uv_fs_write(NULL, &req, file, &buf, 1, -1, NULL);
        uv_fs_req_cleanup(&req);
        
        uv_fs_close(NULL, &req, file, NULL);
        uv_fs_req_cleanup(&req);
    }
    
    serve_static("/", "./test_public", NULL);
}

void cleanup_static(void)
{
    static_cleanup();
    
    uv_fs_t req;
    
    uv_fs_scandir(NULL, &req, "test_public", 0, NULL);
    
    uv_dirent_t dent;
    while (uv_fs_scandir_next(&req, &dent) != UV_EOF)
    {
        char path[512];
        snprintf(path, sizeof(path), "test_public/%s", dent.name);
        
        uv_fs_t unlink_req;
        uv_fs_unlink(NULL, &unlink_req, path, NULL);
        uv_fs_req_cleanup(&unlink_req);
    }
    uv_fs_req_cleanup(&req);
    
    uv_fs_rmdir(NULL, &req, "test_public", NULL);
    uv_fs_req_cleanup(&req);
}
