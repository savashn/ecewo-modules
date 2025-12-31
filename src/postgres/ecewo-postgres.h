#ifndef ECEWO_POSTGRES_H
#define ECEWO_POSTGRES_H
#ifdef __cplusplus
extern "C" {
#endif

#include "libpq-fe.h"
#include "ecewo.h"

typedef struct pg_async_s PGquery;
typedef struct pg_query_s pg_query_t;
typedef void (*pg_result_cb_t)(PGquery *pg, PGresult *result, void *data);

// Create a new query context
// PGquery is automatically destroyed after all queries complete
PGquery *query_create(PGconn *conn, Arena *arena);

// Queue a query for execution
// returns 0 on success, -1 on failure
int query_queue(PGquery *pg,
                const char *sql,
                int param_count,
                const char **params,
                pg_result_cb_t result_cb,
                void *query_data);

// Execute all queued queries
// returns 0 on success, -1 on failure
// PGquery is automatically destroyed after all queries complete
int query_execute(PGquery *pg);

#ifdef __cplusplus
}
#endif
#endif