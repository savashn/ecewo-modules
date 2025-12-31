#include "ecewo-postgres.h"
#include "ecewo.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, "[ERROR] [ecewo-postgres]" fmt "\n", ##__VA_ARGS__)

struct pg_query_s {
    char *sql;
    char **params;
    int param_count;
    pg_result_cb_t result_cb;
    void *data;
    pg_query_t *next;
};

struct pg_async_s {
    PGconn *conn;
    Arena *arena;
    void *data;

    int is_connected;
    int is_executing;

    pg_query_t *query_queue;
    pg_query_t *query_queue_tail;
    pg_query_t *current_query;

    int handle_initialized;
#ifdef _WIN32
    uv_timer_t timer;
#else
    uv_poll_t poll;
#endif
};

static void execute_next_query(PGquery *pg);
static void cleanup_and_destroy(PGquery *pg);

#ifdef _WIN32
static void on_timer(uv_timer_t *handle);
#else
static void on_poll(uv_poll_t *handle, int status, int events);
#endif

static void on_handle_closed(uv_handle_t *handle)
{
    if (!handle || !handle->data)
        return;

    PGquery *pg = (PGquery *)handle->data;
    pg->handle_initialized = 0;
}

static void cancel_execution(PGquery *pg)
{
    if (!pg)
        return;

    if (pg->is_executing && pg->handle_initialized) {
#ifdef _WIN32
        uv_timer_stop(&pg->timer);
        if (!uv_is_closing((uv_handle_t *)&pg->timer)) {
            uv_close((uv_handle_t *)&pg->timer, on_handle_closed);
        }
#else
        uv_poll_stop(&pg->poll);
        if (!uv_is_closing((uv_handle_t *)&pg->poll)) {
            uv_close((uv_handle_t *)&pg->poll, on_handle_closed);
        }
#endif
    }

    if (pg->conn && pg->is_executing) {
        PGcancel *cancel = PQgetCancel(pg->conn);
        if (cancel) {
            char errbuf[256];
            PQcancel(cancel, errbuf, sizeof(errbuf));
            PQfreeCancel(cancel);
        }
    }

    pg->is_executing = 0;
}

static void cleanup_and_destroy(PGquery *pg)
{
    if (!pg)
        return;

    cancel_execution(pg);
    // PGquery is in the arena - caller manages its memory
}

#ifdef _WIN32
static void on_timer(uv_timer_t *handle)
{
    if (!handle || !handle->data)
        return;

    PGquery *pg = (PGquery *)handle->data;

    if (!server_is_running()) {
        uv_timer_stop(&pg->timer);
        if (!uv_is_closing((uv_handle_t *)&pg->timer)) {
            uv_close((uv_handle_t *)&pg->timer, on_handle_closed);
        }
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (!PQconsumeInput(pg->conn)) {
        LOG_ERROR("PQconsumeInput failed: %s", PQerrorMessage(pg->conn));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (PQisBusy(pg->conn))
        return;

    uv_timer_stop(&pg->timer);

    PGresult *result;
    while ((result = PQgetResult(pg->conn)) != NULL) {
        ExecStatusType result_status = PQresultStatus(result);

        if (result_status != PGRES_TUPLES_OK && result_status != PGRES_COMMAND_OK) {
            LOG_ERROR("Query failed: %s", PQresultErrorMessage(result));
            PQclear(result);
            pg->current_query = NULL;
            pg->is_executing = 0;
            decrement_async_work();
            cleanup_and_destroy(pg);
            return;
        }

        if (pg->current_query && pg->current_query->result_cb) {
            pg->current_query->result_cb(pg, result, pg->current_query->data);
        }

        PQclear(result);
    }

    pg->current_query = NULL;
    execute_next_query(pg);
}
#else
static void on_poll(uv_poll_t *handle, int status, int events)
{
    if (!handle || !handle->data)
        return;

    PGquery *pg = (PGquery *)handle->data;

    if (!server_is_running()) {
        uv_poll_stop(&pg->poll);
        if (!uv_is_closing((uv_handle_t *)&pg->poll)) {
            uv_close((uv_handle_t *)&pg->poll, on_handle_closed);
        }
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (status < 0) {
        LOG_ERROR("Poll error: %s", uv_strerror(status));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (!PQconsumeInput(pg->conn)) {
        LOG_ERROR("PQconsumeInput failed: %s", PQerrorMessage(pg->conn));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (PQisBusy(pg->conn))
        return;

    uv_poll_stop(&pg->poll);

    PGresult *result;
    while ((result = PQgetResult(pg->conn)) != NULL) {
        ExecStatusType result_status = PQresultStatus(result);

        if (result_status != PGRES_TUPLES_OK && result_status != PGRES_COMMAND_OK) {
            LOG_ERROR("Query failed: %s", PQresultErrorMessage(result));
            PQclear(result);
            pg->current_query = NULL;
            pg->is_executing = 0;
            decrement_async_work();
            cleanup_and_destroy(pg);
            return;
        }

        if (pg->current_query && pg->current_query->result_cb) {
            pg->current_query->result_cb(pg, result, pg->current_query->data);
        }

        PQclear(result);
    }

    pg->current_query = NULL;
    execute_next_query(pg);
}
#endif

static void execute_next_query(PGquery *pg)
{
    if (!pg->query_queue) {
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (!server_is_running()) {
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    pg->current_query = pg->query_queue;
    pg->query_queue = pg->query_queue->next;
    if (!pg->query_queue) {
        pg->query_queue_tail = NULL;
    }

    int result;
    if (pg->current_query->param_count > 0) {
        result = PQsendQueryParams(
            pg->conn,
            pg->current_query->sql,
            pg->current_query->param_count,
            NULL,
            (const char **)pg->current_query->params,
            NULL,
            NULL,
            0);
    } else {
        result = PQsendQuery(pg->conn, pg->current_query->sql);
    }

    if (!result) {
        LOG_ERROR("Failed to send query: %s", PQerrorMessage(pg->conn));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

#ifdef _WIN32
    if (!pg->handle_initialized) {
        int init_result = uv_timer_init(get_loop(), &pg->timer);
        if (init_result != 0) {
            LOG_ERROR("uv_timer_init failed: %s", uv_strerror(init_result));
            pg->is_executing = 0;
            decrement_async_work();
            cleanup_and_destroy(pg);
            return;
        }
        pg->handle_initialized = 1;
        pg->timer.data = pg;
    }

    int start_result = uv_timer_start(&pg->timer, on_timer, 10, 10);
    if (start_result != 0) {
        LOG_ERROR("uv_timer_start failed: %s", uv_strerror(start_result));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }
#else
    int sock = PQsocket(pg->conn);

    if (sock < 0) {
        LOG_ERROR("Invalid PostgreSQL socket");
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }

    if (!pg->handle_initialized) {
        int init_result = uv_poll_init(get_loop(), &pg->poll, sock);
        if (init_result != 0) {
            LOG_ERROR("uv_poll_init failed: %s", uv_strerror(init_result));
            pg->is_executing = 0;
            decrement_async_work();
            cleanup_and_destroy(pg);
            return;
        }

        pg->handle_initialized = 1;
        pg->poll.data = pg;
    }

    int start_result = uv_poll_start(&pg->poll, UV_READABLE | UV_WRITABLE, on_poll);
    if (start_result != 0) {
        LOG_ERROR("uv_poll_start failed: %s", uv_strerror(start_result));
        pg->is_executing = 0;
        decrement_async_work();
        cleanup_and_destroy(pg);
        return;
    }
#endif
}

PGquery *query_create(PGconn *conn, Arena *arena)
{
    if (!conn || !arena) {
        LOG_ERROR("query_create failed: conn or arena is NULL");
        return NULL;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        LOG_ERROR("query_create failed: Connection status is not OK");
        return NULL;
    }

    PGquery *pg = arena_alloc(arena, sizeof(PGquery));
    if (!pg) {
        LOG_ERROR("query_create failed: Failed to allocate from arena");
        return NULL;
    }

    memset(pg, 0, sizeof(PGquery));
    pg->conn = conn;
    pg->arena = arena;
    pg->is_connected = 1;
    pg->is_executing = 0;
    pg->handle_initialized = 0;
    pg->query_queue = NULL;
    pg->query_queue_tail = NULL;
    pg->current_query = NULL;

#ifdef _WIN32
    pg->timer.data = pg;
#else
    pg->poll.data = pg;
#endif

    return pg;
}

int query_queue(PGquery *pg,
                const char *sql,
                int param_count,
                const char **params,
                pg_result_cb_t result_cb,
                void *query_data)
{
    if (!pg || !sql) {
        LOG_ERROR("query_queue: Invalid parameters");        
        return -1;
    }

    pg_query_t *query = arena_alloc(pg->arena, sizeof(pg_query_t));
    if (!query) {
        LOG_ERROR("query_queue: Failed to allocate query");
        return -1;
    }

    memset(query, 0, sizeof(pg_query_t));
    query->next = NULL;

    query->sql = arena_strdup(pg->arena, sql);
    if (!query->sql) {
        LOG_ERROR("query_queue: Failed to copy SQL");
        return -1;
    }

    if (param_count > 0 && params) {
        query->params = arena_alloc(pg->arena, param_count * sizeof(char *));
        if (!query->params) {
            LOG_ERROR("query_queue: Failed to allocate params");
            return -1;
        }

        for (int i = 0; i < param_count; i++) {
            if (params[i]) {
                query->params[i] = arena_strdup(pg->arena, params[i]);
                if (!query->params[i]) {
                    LOG_ERROR("query_queue: Failed to allocate a param");
                    return -1;
                }
            } else {
                query->params[i] = NULL;
            }
        }
    } else {
        query->params = NULL;
    }

    query->param_count = param_count;
    query->result_cb = result_cb;
    query->data = query_data;

    if (!pg->query_queue) {
        pg->query_queue = pg->query_queue_tail = query;
    } else {
        pg->query_queue_tail->next = query;
        pg->query_queue_tail = query;
    }

    return 0;
}

int query_execute(PGquery *pg)
{
    if (!pg) {
        LOG_ERROR("query_execute: pg is NULL");
        return -1;
    }

    if (!pg->is_connected) {
        LOG_ERROR("query_execute: Not connected");
        return -1;
    }

    if (pg->is_executing)
    {
        LOG_ERROR("query_execute: Already executing");
        return -1;
    }

    if (!pg->query_queue) {
        cleanup_and_destroy(pg);
        return 0;
    }

    increment_async_work();

    pg->is_executing = 1;
    execute_next_query(pg);
    return 0;
}
