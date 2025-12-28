#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-helmet.h"
#include "tester.h"
#include <string.h>

void handler_helmet_test(Req *req, Res *res)
{
    send_text(res, 200, "Helmet OK");
}

int test_helmet_default_headers(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/secure",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };

    MockResponse res = request(&params);

    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Helmet OK", res.body);

    free_request(&res);
    RETURN_OK();
}

int test_helmet_custom_config(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/secure",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };

    MockResponse res = request(&params);

    ASSERT_EQ(200, res.status_code);

    free_request(&res);
    RETURN_OK();
}

void setup_helmet_routes(void)
{
    helmet_init(NULL);
    get("/secure", handler_helmet_test);
}