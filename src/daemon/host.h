/*
 * host.h — Lifecycle owner for the mandatory per-account ANI daemon.
 */
#ifndef ANI_DAEMON_HOST_H
#define ANI_DAEMON_HOST_H

#include "daemon/ipc.h"
#include "daemon/service.h"

#include <stdatomic.h>

typedef struct {
    const ani_daemon_ipc_endpoint_t *endpoint;
    ani_daemon_build_identity_t identity;
    const char *executable_path;
    atomic_int *stop_requested;
    /* Born via `daemon start`: the generation survives its last client
     * disconnect and the no-client initial window; it stops only through the
     * stop/drain ops or an explicit process kill. */
    bool permanent;
} ani_daemon_host_config_t;

/* Blocks for the complete daemon generation. The first admitted frontend owns
 * startup; the final disconnect triggers terminal shutdown of operations,
 * watcher, UI, and runtime in that order. */
int ani_daemon_host_run(const ani_daemon_host_config_t *config);

#endif /* ANI_DAEMON_HOST_H */
