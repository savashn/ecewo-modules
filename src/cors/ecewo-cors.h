#ifndef ECEWO_CORS_H
#define ECEWO_CORS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *origin; // Default: "*"
    const char *methods; // Default: "GET, POST, PUT, DELETE, PATCH, OPTIONS"
    const char *headers; // Default: "Content-Type"
    const char *credentials; // Default: "false"
    const char *max_age; // Default: "3600"
} Cors;

void cors_init(const Cors *config);

#ifdef __cplusplus
}
#endif

#endif
