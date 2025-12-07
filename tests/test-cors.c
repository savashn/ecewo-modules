#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-cors.h"
#include "tester.h"
#include <string.h>

// ============================================================================
// HANDLERS
// ============================================================================

void handler_cors_test(Req *req, Res *res)
{
    send_text(res, 200, "CORS OK");
}

// ============================================================================
// TESTS
// ============================================================================

int test_cors_preflight_request(void)
{
    MockHeaders headers[] = {
        {"Origin", "http://localhost:3000"},
        {"Access-Control-Request-Method", "POST"}
    };
    
    MockParams params = {
        .method = MOCK_OPTIONS,
        .path = "/api/data",
        .body = NULL,
        .headers = headers,
        .header_count = 2
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(204, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

int test_cors_simple_request(void)
{
    MockHeaders headers[] = {
        {"Origin", "http://localhost:3000"}
    };
    
    MockParams params = {
        .method = MOCK_GET,
        .path = "/api/data",
        .body = NULL,
        .headers = headers,
        .header_count = 1
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("CORS OK", res.body);
    
    free_request(&res);
    RETURN_OK();
}

int test_cors_no_origin(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/api/data",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };
    
    MockResponse res = request(&params);
    
    ASSERT_EQ(200, res.status_code);
    
    free_request(&res);
    RETURN_OK();
}

void setup_cors_routes(void)
{
    cors_init(NULL);  // Default CORS
    get("/api/data", handler_cors_test);
}
