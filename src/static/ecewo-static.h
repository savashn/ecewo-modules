#ifndef ECEWO_STATIC_H
#define ECEWO_STATIC_H

#include "ecewo.h"
#include <stdbool.h>

typedef struct
{
    const char *index_file; // Default: "index.html"
    bool enable_etag;       // Enable ETag header, default: 1
    bool enable_cache;      // Enable cache headers, default: 1
    int max_age;            // Cache max-age in seconds, default: 3600
    bool dot_files;         // Serve .dot files? Default: 0 (no)
} Static;

void send_file(Res *res, const char *filepath);

void serve_static(const char *mount_path, // URL prefix (e.g., "/" or "/assets")
                  const char *dir_path,   // Directory path (e.g., "./public")
                  const Static *options); // Configuration options (NULL for defaults)

void static_cleanup(void);

#endif
