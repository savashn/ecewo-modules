# TESTING

The `ecewo-mock.h` file provides a lightweight HTTP mocking module for Ecewo applications. It allows you to test your routes, handlers, and middleware without starting an actual HTTP server.

## Table of Contents

1. [Types](#types)
    1. [`MockMethod`](#mockmethod)
    2. [`MockHeaders`](#mockheaders)
    3. [`MockParams`](#mockparams)
    4. [`MockResponse`](#mockresponse)
2. [Functions](#functions)
    1. [`request()`](#request)
    2. [`free_request()`](#free_request)
    3. [`mock_init()`](#mock_setup)
    4. [`mock_cleanup()`](#mock_cleanup)
3. [Usage](#usage)

> [!NOTE]
>
> This module is for mocking HTTP request only. It doesn't provide assert macros. The assert macros used in this documentation were taken from the [savashn/myassert](https://github.com/savashn/myassert) repository.

## Types

### `MockMethod`

HTTP methods supported by the mock framework:

```c
typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS
} MockMethod;
```

### `MockHeaders`

Represents an HTTP header key-value pair:

```c
typedef struct {
    const char *key;
    const char *value;
} MockHeaders;
```

### `MockParams`

Parameters for creating a mock HTTP request:

```c
typedef struct {
    MockMethod method;      // HTTP method
    const char *path;       // Request path (e.g., "/users/123")
    const char *body;       // Request body (optional, can be NULL)
    MockHeaders *headers;   // Array of headers (optional)
    size_t header_count;    // Number of headers
} MockParams;
```

### `MockResponse`

Response returned by the mock module:

```c
typedef struct {
    uint16_t status_code;   // HTTP status code (e.g., 200, 404)
    char *body;             // Response body
    size_t body_len;        // Length of response body
} MockResponse;
```

## Functions

### `request()`

Execute a mock HTTP request:

```c
MockResponse request(MockParams *params);
```

**Parameters:**

`params`: Pointer to request parameters.

**Returns:**

`MockResponse` containing status code and response body.

**Example:**

```c
MockParams params = {
    .method = GET,
    .path = "/users/123"
};

MockResponse res = request(&params);
ASSERT_EQ(200, res.status_code);
free_request(&res);
```

### `free_request()`

Free memory allocated by a mock response:

```c
void free_request(MockResponse *res);
```

**Parameters:**

`res`: Pointer to response to free.

**Example:**

```c
MockResponse res = request(&params);
// ... use response ...
free_request(&res);  // Always call this!
```

### `mock_init()`

Initialize the mock testing environment:

```c
int mock_init(test_routes_cb_t routes_callback);
```

**Parameters:**

`routes_callback`: A callback that registers the routes.

**Returns:**

`0` on success, non-zero on error

### `mock_cleanup()`

Clean up the mock testing environment:

```c
void mock_cleanup(void);
```

## Usage

```c
#include "ecewo.h"
#include "ecewo-mock.h"
#include "myassert.h"

void handler_new_user(Req *req, Res *res)
{
    const char *authorization = get_header(req, "Authorization");
    const char *content_type = get_header(req, "Content-Type");
    const char *x_custom_header = get_header(req, "X-Custom-Header");

    if (!authorization || !content_type || !x_custom_header)
    {
        send_text(res, BAD_REQUEST, "Missing required headers");
        return;
    }

    if (!req->body || req->body_len == 0)
    {
        send_text(res, BAD_REQUEST, "Empty body");
        return;
    }

    const char *expected_body = "{\"name\":\"John\",\"age\":30}";
    if (strcmp(req->body, expected_body) != 0)
    {
        char *error = arena_sprintf(req->arena, 
            "Body mismatch. Expected: %s, Got: %s", 
            expected_body, req->body);
        send_text(res, BAD_REQUEST, error);
        return;
    }

    send_text(res, CREATED, "Success!");
}

int test_new_user(void)
{
    MockHeaders headers[] = {
        {"Authorization", "Bearer secret-token"},
        {"Content-Type", "application/json"},
        {"X-Custom-Header", "custom-value"}
    };
    
    const char *json_body = "{\"name\":\"John\",\"age\":30}";
    
    MockParams params = {
        .method = MOCK_POST,
        .path = "/new/user",
        .body = json_body,
        .headers = headers,
        .header_count = 3
    };

    MockResponse res = request(&params);

    ASSERT_EQ(201, res.status_code);
    ASSERT_EQ_STR("Success!", res.body);

    free_request(&res);
    
    RETURN_OK();
}

void setup_routes(void)
{
    get("/new/user", handler_new_user);
}

int main(void)
{
    mock_init(setup_routes);

    RUN_TEST(test_new_user);

    mock_cleanup();
    return 0;
}
```
