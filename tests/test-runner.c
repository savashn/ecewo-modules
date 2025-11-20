#include "ecewo.h"
#include "tester.h"
#include "test-handlers.h"

int main(void)
{
    // test cluster
    RUN_TEST(test_cluster_cpu_count);
    RUN_TEST(test_cluster_callbacks);
    RUN_TEST(test_cluster_invalid_config);
    
#ifdef _WIN32
    RUN_TEST(test_cluster_windows_port_strategy);
#else
    RUN_TEST(test_cluster_unix_port_strategy);
#endif
    
    return 0;
}
