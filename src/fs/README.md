# File Operations

Ecewo provides asynchronous file I/O operations using libuv's native API. All operations are non-blocking and run on libuv's thread pool, with automatic memory management and Node.js-style error-first callbacks.

## Table of Contents

1. [Quick Start](#quick-start)
    1. [Reading A File](#reading-a-file)
    2. [Writing A File](#writing-a-file)
2. [File Paths & Working Directory](#file-paths--working-directory)
    1. [Project Structure Example](#project-structure-example)
    2. [CMake Static Files Setup](#cmake-static-files-setup)
    3. [Running From Project Root](#running-from-project-root)
    4. [Running From Build Directory](#running-from-build-directory)
3. [API Reference](#api-reference)
    1. [`fs_read_file()`](#fs_read_file)
    2. [`fs_write_file()`](#fs_write_file)
    3. [`fs_append_file()`](#fs_append_file)
    4. [`fs_stat()`](#fs_stat)
    5. [`fs_unlink()`](#fs_unlink)
    6. [`fs_rename()`](#fs_rename)
    7. [`fs_mkdir()`](#fs_mkdir)
    8. [`fs_rmdir()`](#fs_rmdir)
4. [Advanced Examples](#advanced-examples)
    1. [Sequential File Operations](#sequential-file-operations)
    2. [Parallel File Operations](#parallel-file-operations)
    3. [File Upload Example](#file-upload-example)
5. [Error Handling](#error-handling)
6. [Common Error Codes](#common-error-codes)

> [!IMPORTANT]
>
> File operations use I/O-bound async (libuv), not CPU-bound workers. The main thread is never blocked.

## Quick Start

### Reading A File

```c
#include "ecewo.h"
#include "ecewo-fs.h"

static void on_file_read(const char *error, const char *data, size_t size, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    // Send the public/data.txt file content
    printf("Read %zu bytes\n", size);
    set_header(res, "Content-Type", "text/plain");
    reply(res, OK, data, size);
}

void read_handler(Req *req, Res *res)
{
    // Read the public/data.txt
    // Async file read - main thread is not blocked!
    fs_read_file("public/data.txt", on_file_read, res);
}

int main(void)
{
    server_init();
    get("/file", read_handler);
    server_listen(3000);
    server_run();
    return 0;
}
```

### Writing A File

```c
static void on_file_written(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"saved\"}");
}

void save_handler(Req *req, Res *res)
{
    const char *content = req->body;
    
    fs_write_file("public/output.txt", content, strlen(content), on_file_written, res);
}
```

## File Paths & Working Directory

All file paths are relative to the directory where the server executable is run, not where the executable file is located.

```bash
# If you run server from project root:
/home/user/myproject$ ./build/server

# File paths are relative to /home/user/myproject/
# "data.txt" -> /home/user/myproject/data.txt
# "./logs/app.log" -> /home/user/myproject/logs/app.log
```

```bash
# If you run server from build folder:
/home/user/myproject/build$ ./server

# File paths are relative to /home/user/myproject/build/
# "data.txt" -> /home/user/myproject/build/data.txt
# "./logs/app.log" -> /home/user/myproject/build/logs/app.log
```

```c
// Relative to current working directory
fs_read_file("data.txt", callback, user_data);           // ./data.txt
fs_read_file("logs/app.log", callback, user_data);       // ./logs/app.log
fs_read_file("./config/settings.json", callback, user_data); // ./config/settings.json
```

### Project Structure Example

```
myproject/
├── build/
│   ├── server          # Executable
│   └── public/         # Copied by CMake (see below)
│       ├── index.html
│       └── style.css
├── public/             # Source static files
│   ├── index.html
│   └── style.css
├── data/               # Data files
│   └── users.json
├── logs/               # Log files
│   └── app.log
├── uploads/            # User uploads
├── CMakeLists.txt
└── main.c
```

### CMake Static Files Setup

Add this to your `CMakeLists.txt` to automatically copy/symlink the public/ directory:

```cmake
# Platform-aware public directory handling
if(WIN32)
    # Windows: Copy directory (symlinks require admin privileges)
    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/public
            ${CMAKE_BINARY_DIR}/public
        COMMENT "Copying public directory to build folder"
    )
else()
    # Linux/Mac: Create symlink (faster, no duplication)
    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/public
            ${CMAKE_BINARY_DIR}/public
        COMMENT "Creating symlink to public directory"
    )
endif()
```

**What this does:**

- **Windows:** Copies public/ -> build/public/ on every build
- **Linux/Mac:** Creates symlink build/public/ -> ../public/ (one-time)

Server can be run from either project root or build directory now, because static files are accessible via public/ in both locations.

### Running From Project Root

```bash
cd /home/user/myproject
./build/server
```

**File paths in code:**

```c
// All relative to /home/user/myproject/

fs_read_file("data/users.json", callback, user_data);
// -> /home/user/myproject/data/users.json

fs_write_file("logs/app.log", data, len, callback, user_data);
// -> /home/user/myproject/logs/app.log

fs_write_file("uploads/photo.jpg", img, size, callback, user_data);
// -> /home/user/myproject/uploads/photo.jpg
```

### Running From Build Directory

```bash
cd /home/user/myproject/build
./server
```

**File paths in code:**

```c
// All relative to /home/user/myproject/build/

fs_read_file("../data/users.json", callback, user_data);
// -> /home/user/myproject/data/users.json

fs_write_file("../logs/app.log", data, len, callback, user_data);
// -> /home/user/myproject/logs/app.log

fs_write_file("../uploads/photo.jpg", img, size, callback, user_data);
// -> /home/user/myproject/uploads/photo.jpg
```

## API Reference

### `fs_read_file()`

Read entire file into memory asynchronously.

```c
void fs_read_file(const char *path, fs_read_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: File path to read
- `callback`: Function called when operation completes
- `user_data`: User context pointer (usually `Req*` or `Res*`)

**Callback Signature:**

```c
typedef void (*fs_read_callback_t)(const char *error, const char *data, size_t size, void *user_data);
```

**Callback Parameters:**

- `error`: Error message on failure, `NULL` on success
- `data`: File content on success (user owns this memory, freed when request completes)
- `size`: File size in bytes
- `user_data`: The context pointer you passed

**Example:**

```c
static void on_read(const char *error, const char *data, size_t size, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        char *msg = arena_sprintf(res->arena, "Error: %s", error);
        send_text(res, 500, msg);
        return;
    }
    
    printf("Read %zu bytes\n", size);
    send_text(res, 200, data);
}

void handler(Req *req, Res *res)
{
    fs_read_file("config.json", on_read, res);
}
```

### `fs_write_file()`

Write data to file asynchronously (creates or truncates).

```c
void fs_write_file(const char *path, const void *data, size_t size,
                   fs_write_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: File path to write
- `data`: Data to write
- `size`: Data size in bytes
- `callback`: Completion callback
- `user_data`: User context pointer

**Callback Signature:**

```c
typedef void (*fs_write_callback_t)(const char *error, void *user_data);
```

**Callback Parameters:**

- `error`: Error message on failure, `NULL` on success
- `user_data`: The context pointer you passed

**Example:**

```c
static void on_saved(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"saved\"}");
}

void save_handler(Req *req, Res *res)
{
    const char *json = "{\"status\":\"active\"}";
    
    fs_write_file("status.json", json, strlen(json), on_saved, res);
}
```

### `fs_append_file()`

Append data to file asynchronously (creates if doesn't exist).

```c
void fs_append_file(const char *path, const void *data, size_t size,
                    fs_write_callback_t callback, void *user_data);
```

**Parameters:**

Same as `fs_write_file()`

**Example:**

```c
static void on_logged(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_text(res, 200, "Logged");
}

void log_handler(Req *req, Res *res)
{
    char *log = arena_sprintf(req->arena, "[%ld] %s\n", time(NULL), req->body);
    
    fs_append_file("app.log", log, strlen(log), on_logged, res);
}
```

### `fs_stat()`

Get file statistics asynchronously.

```c
void fs_stat(const char *path, fs_stat_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: File path to stat
- `callback`: Completion callback
- `user_data`: User context pointer

**Callback Signature:**

```c
typedef void (*fs_stat_callback_t)(const char *error, const uv_stat_t *stat, void *user_data);
```

**Callback Parameters:**

- `error`: Error message on failure, `NULL` on success
- `stat`: File statistics on success
- `user_data`: The context pointer you passed

**Available stat fields:**

- `st_size`: File size in bytes
- `st_mtim`: Last modification time
- `st_atim`: Last access time
- `st_ctim`: Creation time
- `st_mode`: File permissions

**Example:**

```c
static void on_stat(const char *error, const uv_stat_t *stat, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 404, "File not found");
        return;
    }
    
    char *response = arena_sprintf(res->arena,
        "{"
        "\"size\":%lld,"
        "\"modified\":%lld"
        "}",
        (long long)stat->st_size,
        (long long)stat->st_mtim.tv_sec
    );
    
    send_json(res, 200, response);
}

void info_handler(Req *req, Res *res)
{
    const char *path = get_query(req, "path");
    fs_stat(path, on_stat, res);
}
```

### `fs_unlink()`

Delete file asynchronously.

```c
void fs_unlink(const char *path, fs_write_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: File path to delete
- `callback`: Completion callback (same as write callback)
- `user_data`: User context pointer

**Example:**

```c
static void on_deleted(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"deleted\"}");
}

void delete_handler(Req *req, Res *res)
{
    const char *file = get_query(req, "file");
    
    fs_unlink(file, on_deleted, res);
}
```

### `fs_rename()`

Rename or move file asynchronously.

```c
void fs_rename(const char *old_path, const char *new_path,
               fs_write_callback_t callback, void *user_data);
```

**Parameters:**

- `old_path`: Current file path
- `new_path`: New file path
- `callback`: Completion callback
- `user_data`: User context pointer

**Example:**

```c
static void on_renamed(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"renamed\"}");
}

void rename_handler(Req *req, Res *res)
{
    fs_rename("old.txt", "new.txt", on_renamed, res);
}
```

### `fs_mkdir()`

Create directory asynchronously.

```c
void fs_mkdir(const char *path, fs_write_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: Directory path to create
- `callback`: Completion callback
- `user_data`: User context pointer

**Example:**

```c
static void on_dir_created(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"created\"}");
}

void create_dir_handler(Req *req, Res *res)
{
    fs_mkdir("uploads", on_dir_created, res);
}
```

### `fs_rmdir()`

Remove empty directory asynchronously.

```c
void fs_rmdir(const char *path, fs_write_callback_t callback, void *user_data);
```

**Parameters:**

- `path`: Directory path to remove
- `callback`: Completion callback
- `user_data`: User context pointer

**Example:**

```c
static void on_dir_removed(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"removed\"}");
}

void remove_dir_handler(Req *req, Res *res)
{
    fs_rmdir("temp", on_dir_removed, res);
}
```

## Advanced Examples

### Sequential File Operations

```c
// Step 2: Write processed content
static void on_written(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;

    if (error)
    {
        send_text(res, 500, error);
        return;
    }

    send_text(res, 200, "Processed and saved");
}

// Step 1: Read and process
static void on_read(const char *error, const char *data, size_t size, void *user_data)
{
    Res *res = (Res *)user_data;

    if (error)
    {
        send_text(res, 500, error);
        return;
    }

    // Process file content
    char *processed = arena_sprintf(res->arena, "PROCESSED: %s", data);

    // Write processed content
    fs_write_file("output.txt", processed, strlen(processed), on_written, res);
}

void process_handler(Req *req, Res *res)
{
    // Read, process, and write
    fs_read_file("public/input.txt", on_read, res);
}
```

### Parallel File Operations

```c
#include "ecewo.h"
#include "ecewo-fs.h"

typedef struct
{
    Res *res;
    int completed;
    int total;
    char *file1_data;
    char *file2_data;
    char *file3_data;
} ParallelContext;

static void send_combined_response(ParallelContext *ctx)
{
    if (ctx->completed == ctx->total)
    {
        char *response = arena_sprintf(ctx->res->arena,
            "{"
            "\"file1\":\"%s\","
            "\"file2\":\"%s\","
            "\"file3\":\"%s\""
            "}",
            ctx->file1_data ? ctx->file1_data : "error",
            ctx->file2_data ? ctx->file2_data : "error",
            ctx->file3_data ? ctx->file3_data : "error"
        );

        send_json(ctx->res, 200, response);
    }
}

static void on_file1(const char *error, const char *data, size_t size, void *user_data)
{
    ParallelContext *ctx = (ParallelContext *)user_data;

    if (!error)
    {
        ctx->file1_data = arena_strdup(ctx->res->arena, data);
    }

    ctx->completed++;
    send_combined_response(ctx);
}

static void on_file2(const char *error, const char *data, size_t size, void *user_data)
{
    ParallelContext *ctx = (ParallelContext *)user_data;

    if (!error)
    {
        ctx->file2_data = arena_strdup(ctx->res->arena, data);
    }

    ctx->completed++;
    send_combined_response(ctx);
}

static void on_file3(const char *error, const char *data, size_t size, void *user_data)
{
    ParallelContext *ctx = (ParallelContext *)user_data;

    if (!error)
    {
        ctx->file3_data = arena_strdup(ctx->res->arena, data);
    }

    ctx->completed++;
    send_combined_response(ctx);
}

void parallel_handler(Req *req, Res *res)
{
    ParallelContext *ctx = arena_alloc(req->arena, sizeof(ParallelContext));
    ctx->res = res;
    ctx->completed = 0;
    ctx->total = 3;
    ctx->file1_data = NULL;
    ctx->file2_data = NULL;
    ctx->file3_data = NULL;

    // Start 3 parallel reads - all callbacks receive the same context
    fs_read_file("public/file1.txt", on_file1, ctx);
    fs_read_file("public/file2.txt", on_file2, ctx);
    fs_read_file("public/file3.txt", on_file3, ctx);
}
```

### File Upload Example

```c
static void on_uploaded(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        send_text(res, 500, error);
        return;
    }
    
    send_json(res, 200, "{\"status\":\"uploaded\"}");
}

void upload_handler(Req *req, Res *res)
{
    // Get filename from request params
    const char *filename = get_param(req, "filename");
    if (!filename)
    {
        send_text(res, 400, "Missing filename");
        return;
    }
    
    // Build safe path
    char *filepath = arena_sprintf(req->arena, "uploads/%s", filename);
    
    // Save uploaded file
    fs_write_file(filepath, req->body, req->body_len, on_uploaded, res);
}
```

## Error Handling

```c
static void on_operation(const char *error, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (error)
    {
        // Error occurred
        // error contains: "ENOENT: no such file or directory"
        
        if (strstr(error, "ENOENT"))
        {
            send_text(res, 404, "File not found");
        }
        else if (strstr(error, "EACCES"))
        {
            send_text(res, 403, "Permission denied");
        }
        else if (strstr(error, "EISDIR"))
        {
            send_text(res, 400, "Path is a directory");
        }
        else
        {
            send_text(res, 500, error);
        }
        
        return;
    }
    
    // Success
    send_text(res, 200, "OK");
}
```

## Common Error Codes

| Error Code | Meaning                 | HTTP Status |
|------------|-------------------------|-------------|
| `ENOENT`   | File not found          | 404         |
| `EACCES`   | Permission denied       | 403         |
| `EISDIR`   | Is a directory          | 400         |
| `ENOTDIR`  | Not a directory         | 400         |
| `EEXIST`   | File already exists     | 409         |
| `ENOSPC`   | No space left on device | 507         |

## Memory Management

All file operations automatically manage memory:

- **Read operations:** The `data` pointer in the callback is owned by the user and automatically freed when the request/response completes
- **Write operations:** Internal buffers are automatically freed after the operation completes
- **No manual cleanup required:** You don't need to free anything returned by file operations

```c
static void on_read(const char *error, const char *data, size_t size, void *user_data)
{
    Res *res = (Res *)user_data;
    
    if (!error)
    {
        // Use data directly - no need to free it
        send_text(res, 200, data);
        
        // Or copy to arena if you need it longer
        char *copy = arena_strdup(res->arena, data);
    }
    
    // data is automatically freed when response completes
}
```

## Key Differences from V1

1. **Cleaner callbacks:** Each operation has its own callback type
   - `fs_read_callback_t` for read operations
   - `fs_write_callback_t` for write/delete/mkdir/rmdir operations
   - `fs_stat_callback_t` for stat operations

2. **Better ergonomics:** User data is passed as the last parameter
   ```c
   // V1: fs_read_file(res, "file.txt", callback);
   // V2: fs_read_file("file.txt", callback, res);
   ```

3. **No exposed internal structures:** All implementation details are hidden

4. **Type safety:** Compile-time type checking for callbacks

5. **Node.js compatibility:** Error-first callback pattern matches Node.js fs module
