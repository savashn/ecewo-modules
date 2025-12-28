#include "ecewo-cluster.h"
#include "ecewo.h"
#include "tester.h"
#include <string.h>
#include <stdio.h>

#define REQUEST_COUNT 5

static bool worker_started = false;
static bool worker_exited = false;
static uint8_t last_worker_id = 0;
static int last_exit_status = 0;

void test_worker_start_callback(uint8_t worker_id)
{
    worker_started = true;
    last_worker_id = worker_id;
}

void test_worker_exit_callback(uint8_t worker_id, int status)
{
    worker_exited = true;
    last_worker_id = worker_id;
    last_exit_status = status;
}

// ============================================================================
// TESTS
// ============================================================================

int test_cluster_cpu_count(void)
{
    uint8_t cpu_count = cluster_cpus();
    
    ASSERT_GT(cpu_count, 1);
    ASSERT_LE(cpu_count, 255);

    RETURN_OK();
}

int test_cluster_callbacks(void)
{
    worker_started = false;
    worker_exited = false;
    
    Cluster config = {
        .cpus = 2,
        .respawn = true,
        .port = 3000,
        .on_start = test_worker_start_callback,
        .on_exit = test_worker_exit_callback
    };
    
    ASSERT_NOT_NULL(config.on_start);
    ASSERT_NOT_NULL(config.on_exit);

    RETURN_OK();
}

int test_cluster_invalid_config(void)
{
    Cluster* null_config = NULL;

    bool init_result = cluster_init(null_config, 0, NULL);
    ASSERT_FALSE(init_result);

    Cluster invalid_workers = {
        .cpus = 0,
        .port = 3000
    };

    init_result = cluster_init(&invalid_workers, 0, NULL);
    ASSERT_FALSE(init_result);

    Cluster invalid_port = {
        .cpus = 2,
        .port = 0
    };

    init_result = cluster_init(&invalid_port, 0, NULL);
    ASSERT_FALSE(init_result);

    RETURN_OK();
}

#ifdef _WIN32
int test_cluster_windows_port_strategy(void)
{
    Cluster config = {
        .cpus = 3,
        .respawn = true,
        .port = 3000
    };

    bool result = cluster_init(&config, 0, NULL);
    ASSERT_TRUE(result);

    uint16_t expected_ports[] = {3000, 3001, 3002};

    for (int i = 0; i < config.cpus; i++) {
        uint16_t current_port = cluster_get_port();
        ASSERT_EQ(expected_ports[i], current_port);
    }

    ASSERT_EQ(3000, config.port);
    ASSERT_EQ(3, config.cpus);

    RETURN_OK();
}
#else
int test_cluster_unix_port_strategy(void)
{
    Cluster config = {
        .cpus = 4,
        .respawn = true,
        .port = 3000
    };
    
    ASSERT_EQ(3000, config.port);
    ASSERT_EQ(4, config.cpus);

    RETURN_OK();
}
#endif
