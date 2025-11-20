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
    3. [`test_routes_hook()`](#test_routes_hook)
    4. [`mock_setup()`](#mock_setup)
    5. [`mock_down()`](#mock_down)
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
    PATCH
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
    MockMethod method;          // HTTP method
    const char *path;           // Request path (e.g., "/users/123")
    const char *body;           // Request body (optional, can be NULL)
    MockHeaders *headers;       // Array of headers (optional)
    size_t header_count;        // Number of headers
} MockParams;
```

### `MockResponse`

Response returned by the mock module:

```c
typedef struct {
    uint16_t status_code;       // HTTP status code (e.g., 200, 404)
    char *body;                 // Response body
    size_t body_len;            // Length of response body
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

### `test_routes_hook()`

Register routes for testing:

```c
void test_routes_hook(test_routes_cb_t callback);
```

**Parameters:**

`callback`: Function that sets up routes

**Example:**

```c
void setup_routes(void) {
    get("/hello", hello_handler);
    post("/users", create_user);
}

int main(void) {
    test_routes_hook(setup_routes);
    // ... run tests ...
}
```

### `mock_setup()`

Initialize the mock testing environment:

```c
int mock_setup(void);
```

**Returns:**

`0` on success, non-zero on error

### `mock_down()`

Clean up the mock testing environment:

```c
void mock_down(void);
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
        char *error = ecewo_sprintf(res, 
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
    test_routes_hook(setup_routes);
    mock_setup();

    RUN_TEST(test_new_user);

    mock_down();
    return 0;
}
```
