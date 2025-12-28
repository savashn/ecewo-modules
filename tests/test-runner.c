#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-session.h"
#include "tester.h"
#include "test-handlers.h"

void setup_all_routes(void)
{
    setup_cookie_routes();
    setup_cors_routes();
    setup_helmet_routes();
    setup_session_routes();
    setup_fs_routes();
    setup_static_routes();
}

int main(void)
{
    printf("--- Cluster Tests ---\n");
    RUN_TEST(test_cluster_cpu_count);
    RUN_TEST(test_cluster_callbacks);
    RUN_TEST(test_cluster_invalid_config);
#ifdef _WIN32
    RUN_TEST(test_cluster_windows_port_strategy);
#else
    RUN_TEST(test_cluster_unix_port_strategy);
#endif
    
    printf("\n--- Session Unit Tests ---\n");
    session_init();
    RUN_TEST(test_session_value_set_get);
    RUN_TEST(test_session_value_overwrite);
    RUN_TEST(test_session_value_remove);
    RUN_TEST(test_session_find);
    RUN_TEST(test_session_utf8_values);
    session_cleanup();
    
    printf("\n--- HTTP Integration Tests ---\n");
    
    if (mock_init(setup_all_routes) != 0)
    {
        printf("ERROR: Failed to initialize mock server\n");
        return 1;
    }

    printf("DEBUG: Mock initialized, routes should be set up\n");
    fflush(stdout);
    
    printf("\n--- Cookie Tests ---\n");
    RUN_TEST(test_cookie_set_simple);
    RUN_TEST(test_cookie_set_complex);
    RUN_TEST(test_cookie_get);
    RUN_TEST(test_cookie_get_not_found);
    RUN_TEST(test_cookie_get_multiple);
    RUN_TEST(test_cookie_delete);
    RUN_TEST(test_cookie_url_encoded);
    
    printf("\n--- CORS Tests ---\n");
    RUN_TEST(test_cors_simple_request);
    RUN_TEST(test_cors_no_origin);
    
    printf("\n--- Helmet Tests ---\n");
    RUN_TEST(test_helmet_default_headers);
    RUN_TEST(test_helmet_custom_config);
    
    printf("\n--- Session HTTP Tests ---\n");
    RUN_TEST(test_session_create);
    RUN_TEST(test_session_no_session);
    
    printf("\n--- File System Tests ---\n");
    RUN_TEST(test_fs_read_existing_file);
    printf("DEBUG: After first FS test\n");
    fflush(stdout);
    RUN_TEST(test_fs_read_nonexistent_file);
    RUN_TEST(test_fs_write_file);
    RUN_TEST(test_fs_stat_file);
    RUN_TEST(test_fs_missing_parameter);
    
    printf("\n--- Static File Tests ---\n");
    fflush(stdout);
    printf("DEBUG: About to run static tests\n");
    fflush(stdout);
    RUN_TEST(test_static_serve_html);
    RUN_TEST(test_static_serve_index);
    RUN_TEST(test_static_not_found);
    RUN_TEST(test_static_dotfile_blocked);
    RUN_TEST(test_static_path_traversal_blocked);
    
    cleanup_session();
    cleanup_fs();
    cleanup_static();
    mock_cleanup();
    
    printf("\n========================================\n");
    printf("         ALL TESTS COMPLETED\n");
    printf("========================================\n\n");
    
    return 0;
}
