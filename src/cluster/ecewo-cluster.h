#ifndef ECEWO_CLUSTER_H
#define ECEWO_CLUSTER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t cpus;
    bool respawn;
    uint16_t port;
    void (*on_start)(uint8_t worker_id);
    void (*on_exit)(uint8_t worker_id, int status);
} Cluster;

bool cluster_init(const Cluster *config, int argc, char **argv);
uint16_t cluster_get_port(void);
bool cluster_is_master(void);
bool cluster_is_worker(void);
uint8_t cluster_worker_id(void);
uint8_t cluster_worker_count(void);
uint8_t cluster_cpus(void);
uint8_t cluster_cpus_physical(void);
void cluster_signal_workers(int signal);
void cluster_wait_workers(void);

#endif
