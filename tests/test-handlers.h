#ifndef TEST_HANDLERS_H
#define TEST_HANDLERS_H

// cluster
int test_cluster_cpu_count(void);
int test_cluster_callbacks(void);
int test_cluster_invalid_config(void);
int test_cluster_port_strategy(void);

// cookie
int test_cookie_set_simple(void);
int test_cookie_set_complex(void);
int test_cookie_get(void);
int test_cookie_get_not_found(void);
int test_cookie_get_multiple(void);
int test_cookie_delete(void);
int test_cookie_url_encoded(void);
void setup_cookie_routes(void);

// cors
int test_cors_preflight_request(void);
int test_cors_simple_request(void);
int test_cors_no_origin(void);
void setup_cors_routes(void);

// helmet
int test_helmet_default_headers(void);
int test_helmet_custom_config(void);
void setup_helmet_routes(void);

// session
int test_session_create(void);
int test_session_no_session(void);
int test_session_value_set_get(void);
int test_session_value_overwrite(void);
int test_session_value_remove(void);
int test_session_find(void);
int test_session_utf8_values(void);
void setup_session_routes(void);
void cleanup_session(void);

// fs
int test_fs_read_existing_file(void);
int test_fs_read_nonexistent_file(void);
int test_fs_write_file(void);
int test_fs_stat_file(void);
int test_fs_missing_parameter(void);
void setup_fs_routes(void);
void cleanup_fs(void);

// static
int test_static_serve_html(void);
int test_static_serve_index(void);
int test_static_not_found(void);
int test_static_dotfile_blocked(void);
int test_static_path_traversal_blocked(void);
void setup_static_routes(void);
void cleanup_static(void);

#endif
