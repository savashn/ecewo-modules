#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-cookie.h"
#include "tester.h"
#include <string.h>

// ============================================================================
// HANDLERS
// ============================================================================

void handler_set_simple_cookie(Req *req, Res *res)
{
    cookie_set(res, "theme", "dark", NULL);
    send_text(res, 200, "Cookie set");
}

void handler_set_complex_cookie(Req *req, Res *res)
{
    Cookie opts = {
        .max_age = 3600,
        .path = "/",
        .same_site = "Strict",
        .http_only = true,
        .secure = false
    };
    
    cookie_set(res, "session_id", "abc123", &opts);
    send_text(res, 200, "Complex cookie set");
}

void handler_get_cookie(Req *req, Res *res)
{
    char *value = cookie_get(req, "user");
    if (value)
    {
        send_text(res, 200, value);
    }
    else
    {
        send_text(res, 404, "Cookie not found");
    }
}

void handler_delete_cookie(Req *req, Res *res)
{
    Cookie opts = {
        .max_age = 0
    };
    
    cookie_set(res, "session_id", "", &opts);
    send_text(res, 200, "Cookie deleted");
}

void handler_utf8_cookie(Req *req, Res *res)
{
    cookie_set(res, "greeting", "merhaba dünya", NULL);
    send_text(res, 200, "UTF-8 cookie set");
}

// ============================================================================
// TESTS
// ============================================================================

int test_cookie_set_simple(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/set-simple",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Cookie set", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_set_complex(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/set-complex",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Complex cookie set", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_get(void)
{
    MockHeaders headers[] = {
        {"Cookie", "user=john_doe"}
    };
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/get-cookie",
        .body = NULL,
        .headers = headers,
        .header_count = 1
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("john_doe", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_get_not_found(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/get-cookie",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(404, res.status_code);
    ASSERT_EQ_STR("Cookie not found", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_get_multiple(void)
{
    MockHeaders headers[] = {
        {"Cookie", "first=one; user=target_value; last=three"}
    };
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/get-cookie",
        .body = NULL,
        .headers = headers,
        .header_count = 1
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("target_value", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_delete(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/delete-cookie",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Cookie deleted", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cookie_url_encoded(void)
{
    MockHeaders headers[] = {
        {"Cookie", "user=hello%20world"}
    };
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/get-cookie",
        .body = NULL,
        .headers = headers,
        .header_count = 1
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("hello world", res.body);
    
    free_request(&res);
    RETURN_OK();
}

// ============================================================================
// SETUP
// ============================================================================

void setup_cookie_routes(void)
{
    get("/set-simple", handler_set_simple_cookie);
    get("/set-complex", handler_set_complex_cookie);
    get("/get-cookie", handler_get_cookie);
    get("/delete-cookie", handler_delete_cookie);
    get("/utf8-cookie", handler_utf8_cookie);
}
