#ifndef ECEWO_POSTGRES_H
#define ECEWO_POSTGRES_H

#include "libpq-fe.h"

// Forward declarations
typedef struct pg_async_s PGquery;
typedef struct pg_query_s pg_query_t;

// Result callback function type
typedef void (*pg_result_cb_t)(PGquery *pg, PGresult *result, void *data);

// Create new async PostgreSQL query context
PGquery *query_create(PGconn *existing_conn, void *data);

// Add query to execution queue
int query_queue(PGquery *pg,              // Query context from query_create()
                const char *sql,          // SQL query string (will be copied)
                int param_count,          // Number of parameters
                const char **params,      // Parameter values (will be copied)
                pg_result_cb_t result_cb, // Callback for query results
                void *query_data);        // User data for this specific query

// Start executing queued queries asynchronously
int query_execute(PGquery *pg);

#endif
