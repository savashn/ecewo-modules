#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-session.h"
#include "tester.h"
#include <string.h>

// ============================================================================
// HANDLERS
// ============================================================================

void handler_session_create(Req *req, Res *res)
{
    Session *sess = session_create(3600);
    if (!sess) {
        send_text(res, 500, "Session creation failed");
        return;
    }

    session_value_set(sess, "user_id", "12345");
    session_value_set(sess, "username", "john");

    session_send(res, sess, NULL);
    send_text(res, 200, "Session created");
}

void handler_session_get(Req *req, Res *res)
{
    Session *sess = session_get(req);
    if (!sess) {
        send_text(res, 401, "No session");
        return;
    }

    char *user_id = session_value_get(sess, "user_id");
    if (user_id) {
        send_text(res, 200, user_id);
        free(user_id);
    } else {
        send_text(res, 404, "user_id not found");
    }
}

void handler_session_destroy(Req *req, Res *res)
{
    Session *sess = session_get(req);
    if (sess) {
        session_destroy(res, sess, NULL);
    }
    send_text(res, 200, "Session destroyed");
}

// ============================================================================
// TESTS
// ============================================================================

int test_session_create(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/session/create",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };

    MockResponse res = request(&params);

    ASSERT_EQ(200, res.status_code);
    ASSERT_EQ_STR("Session created", res.body);

    free_request(&res);
    RETURN_OK();
}

int test_session_no_session(void)
{
    MockParams params = {
        .method = MOCK_GET,
        .path = "/session/get",
        .body = NULL,
        .headers = NULL,
        .header_count = 0
    };

    MockResponse res = request(&params);

    ASSERT_EQ(401, res.status_code);
    ASSERT_EQ_STR("No session", res.body);

    free_request(&res);
    RETURN_OK();
}

int test_session_value_set_get(void)
{
    Session *sess = session_create(3600);
    ASSERT_NOT_NULL(sess);

    session_value_set(sess, "key1", "value1");
    session_value_set(sess, "key2", "value2");

    char *val1 = session_value_get(sess, "key1");
    char *val2 = session_value_get(sess, "key2");
    char *val3 = session_value_get(sess, "nonexistent");

    ASSERT_NOT_NULL(val1);
    ASSERT_NOT_NULL(val2);
    ASSERT_NULL(val3);

    ASSERT_EQ_STR("value1", val1);
    ASSERT_EQ_STR("value2", val2);

    free(val1);
    free(val2);

    session_free(sess);
    RETURN_OK();
}

int test_session_value_overwrite(void)
{
    Session *sess = session_create(3600);
    ASSERT_NOT_NULL(sess);

    session_value_set(sess, "key", "first");
    session_value_set(sess, "key", "second");

    char *val = session_value_get(sess, "key");
    ASSERT_NOT_NULL(val);
    ASSERT_EQ_STR("second", val);

    free(val);
    session_free(sess);
    RETURN_OK();
}

int test_session_value_remove(void)
{
    Session *sess = session_create(3600);
    ASSERT_NOT_NULL(sess);

    session_value_set(sess, "to_remove", "value");

    char *before = session_value_get(sess, "to_remove");
    ASSERT_NOT_NULL(before);
    ASSERT_EQ_STR("value", before);
    free(before);

    session_value_remove(sess, "to_remove");

    char *after = session_value_get(sess, "to_remove");
    ASSERT_NULL(after);

    session_free(sess);
    RETURN_OK();
}

int test_session_find(void)
{
    Session *sess = session_create(3600);
    ASSERT_NOT_NULL(sess);

    char id_copy[SESSION_ID_LEN + 1];
    strncpy(id_copy, sess->id, SESSION_ID_LEN);
    id_copy[SESSION_ID_LEN] = '\0';

    Session *found = session_find(id_copy);
    ASSERT_NOT_NULL(found);
    ASSERT_EQ_STR(sess->id, found->id);

    Session *not_found = session_find("nonexistent_session_id_12345");
    ASSERT_NULL(not_found);

    session_free(sess);
    RETURN_OK();
}

int test_session_utf8_values(void)
{
    Session *sess = session_create(3600);
    ASSERT_NOT_NULL(sess);

    session_value_set(sess, "turkish", "merhaba dünya");
    session_value_set(sess, "emoji", "test 🎉");

    char *turkish = session_value_get(sess, "turkish");
    ASSERT_NOT_NULL(turkish);
    ASSERT_EQ_STR("merhaba dünya", turkish);
    free(turkish);

    char *emoji = session_value_get(sess, "emoji");
    ASSERT_NOT_NULL(emoji);
    ASSERT_EQ_STR("test 🎉", emoji);
    free(emoji);

    session_free(sess);
    RETURN_OK();
}

// ============================================================================
// SETUP
// ============================================================================

void setup_session_routes(void)
{
    session_init();
    get("/session/create", handler_session_create);
    get("/session/get", handler_session_get);
    get("/session/destroy", handler_session_destroy);
}

void cleanup_session(void)
{
    session_cleanup();
}
