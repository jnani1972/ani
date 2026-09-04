/*
 * bootstrap.h — Process-role policy and mandatory daemon bootstrap.
 *
 * Role classification must happen before any stateful initialization in
 * main(): no store, watcher, UI, diagnostics, or index supervisor may be
 * constructed until the process is known to be the daemon, one of its
 * internal workers, a thin client, or an explicitly stateless command.
 */
#ifndef ANI_DAEMON_BOOTSTRAP_H
#define ANI_DAEMON_BOOTSTRAP_H

#include "daemon/runtime.h"
#include "daemon/version_cohort.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ANI_DAEMON_INTERNAL_ARG "--ani-daemon-internal"
#define ANI_DAEMON_PERMANENT_ARG "--ani-daemon-permanent"
#define ANI_DAEMON_BOOTSTRAP_LAUNCH_ARGC 3U

typedef enum {
    ANI_DAEMON_PROCESS_INVALID = 0,
    ANI_DAEMON_PROCESS_STATELESS,
    ANI_DAEMON_PROCESS_DAEMON,
    ANI_DAEMON_PROCESS_WORKER,
    ANI_DAEMON_PROCESS_MCP_CLIENT,
    ANI_DAEMON_PROCESS_LOCAL_CLI,
    ANI_DAEMON_PROCESS_HOOK_CLIENT,
    ANI_DAEMON_PROCESS_DAEMON_CTL,
} ani_daemon_process_role_t;

/* Pure argv classifier. argv[0] is the executable. The hidden daemon role is
 * accepted only as the sole argument; index workers must use the exact
 * build-bound `cli --index-worker ...` grammar. */
ani_daemon_process_role_t ani_daemon_process_role(int argc, char *const argv[]);

/* True only for externally launched long-lived frontends. Internal daemon and
 * worker processes plus one-shot local CLI calls never count as client leases. */
bool ani_daemon_process_role_requires_client(ani_daemon_process_role_t role);

/* Construct the one stable per-account endpoint. The endpoint identity is the
 * product rendezvous key: executable path, semantic version, build hash, and
 * ABI values must never create parallel daemon namespaces. */
ani_daemon_ipc_endpoint_t *ani_daemon_bootstrap_endpoint_new(const char *runtime_parent);

/* Cross-platform launch policy for the daemon child. The child is invoked
 * directly (never through a shell), with exactly argv[0] plus the one hidden
 * internal argument. It is detached from the launching client's lifetime and
 * inherits no standard handles; logical client leases govern its lifetime. */
typedef struct {
    const char *executable_path;
    const char *argv[ANI_DAEMON_BOOTSTRAP_LAUNCH_ARGC + 1U];
    size_t argc;
    bool detached;
    bool inherit_standard_handles;
    bool use_shell;
} ani_daemon_bootstrap_launch_spec_t;

bool ani_daemon_bootstrap_launch_spec_init(const char *executable_path,
                                           ani_daemon_bootstrap_launch_spec_t *spec_out);

/* Same launch policy, but the child is born PERMANENT: it survives its last
 * client disconnect and stops only via `daemon stop`, the install/update
 * drain, or an explicit process kill. Only `daemon start` uses this. */
bool ani_daemon_bootstrap_launch_spec_init_permanent(const char *executable_path,
                                                     ani_daemon_bootstrap_launch_spec_t *spec_out);

typedef enum {
    ANI_DAEMON_BOOTSTRAP_FAILED = 0,
    ANI_DAEMON_BOOTSTRAP_BYPASSED,
    ANI_DAEMON_BOOTSTRAP_CONNECTED,
    ANI_DAEMON_BOOTSTRAP_CONFLICT,
} ani_daemon_bootstrap_status_t;

typedef struct {
    ani_daemon_process_role_t role;
    const ani_daemon_ipc_endpoint_t *endpoint;
    const ani_daemon_build_identity_t *identity;
    const char *executable_path;
    uint32_t connect_timeout_ms;
    uint32_t startup_timeout_ms;
    /* `daemon start` only: an absent endpoint is replaced by a PERMANENT
     * generation instead of an ephemeral one. Client bootstraps leave this
     * false — they must never mint permanence implicitly. */
    bool spawn_permanent;
} ani_daemon_bootstrap_config_t;

typedef struct {
    ani_daemon_bootstrap_status_t status;
    ani_daemon_runtime_client_t *client;
    ani_daemon_runtime_connect_result_t connect_result;
    bool daemon_spawned;
    char message[ANI_DAEMON_CONFLICT_MESSAGE_SIZE];
} ani_daemon_bootstrap_result_t;

/* A probe distinguishes an absent endpoint from a reserved endpoint whose
 * listener is STARTING, saturated, or otherwise temporarily unable to admit
 * this client. RESERVED and TERMINAL are wait states for the observed
 * generation. If it later becomes truly absent, this same attempt re-enters
 * startup serialization and may launch exactly one replacement. */
typedef enum {
    ANI_DAEMON_BOOTSTRAP_PROBE_ERROR = 0,
    ANI_DAEMON_BOOTSTRAP_PROBE_UNAVAILABLE,
    ANI_DAEMON_BOOTSTRAP_PROBE_RESERVED,
    ANI_DAEMON_BOOTSTRAP_PROBE_CONNECTED,
    ANI_DAEMON_BOOTSTRAP_PROBE_CONFLICT,
    ANI_DAEMON_BOOTSTRAP_PROBE_TERMINAL,
} ani_daemon_bootstrap_probe_status_t;

/* Deterministic policy seam shared by production probing and unit contracts.
 * lifetime_status follows the IPC reservation tri-state (1 held, 0 free,
 * -1 error). Every protocol-level REJECTED response is RESERVED unless its
 * message explicitly reports terminal shutdown; it is never UNAVAILABLE. */
ani_daemon_bootstrap_probe_status_t ani_daemon_bootstrap_classify_failed_connect(
    const ani_daemon_runtime_connect_result_t *connect_result, int lifetime_status);

typedef void *ani_daemon_bootstrap_lock_t;
typedef void *ani_daemon_bootstrap_cohort_t;

/* Injectable OS/runtime boundary used by the deterministic unit contract.
 * Production callers use ani_daemon_bootstrap_execute(), whose built-in
 * operations delegate to daemon IPC/runtime and write visible diagnostics to
 * stderr. Lock acquisition uses the IPC tri-state convention: 1 acquired,
 * 0 held by another starter, -1 error. */
typedef struct {
    void *context;
    ani_version_cohort_status_t (*cohort_acquire)(void *context,
                                                  const ani_daemon_ipc_endpoint_t *endpoint,
                                                  const ani_daemon_build_identity_t *identity,
                                                  uint64_t deadline_ms,
                                                  ani_daemon_bootstrap_cohort_t *cohort_out,
                                                  ani_daemon_conflict_t *conflict_out);
    void (*cohort_release)(void *context, ani_daemon_bootstrap_cohort_t cohort);
    ani_daemon_bootstrap_probe_status_t (*probe)(void *context,
                                                 const ani_daemon_ipc_endpoint_t *endpoint,
                                                 const ani_daemon_build_identity_t *identity,
                                                 uint32_t timeout_ms,
                                                 ani_daemon_runtime_client_t **client_out,
                                                 ani_daemon_runtime_connect_result_t *result_out);
    int (*startup_lock_try_acquire)(void *context, const ani_daemon_ipc_endpoint_t *endpoint,
                                    ani_daemon_bootstrap_lock_t *lock_out);
    /* Called with startup serialization still held immediately before spawn.
     * It releases only migration-era compatibility ownership that the child
     * must reacquire for its lifetime; the ordinary startup lock stays held. */
    bool (*startup_lock_prepare_handoff)(void *context, ani_daemon_bootstrap_lock_t lock);
    /* Retry-safe: success consumes and clears *lock_io; false retains it. */
    bool (*startup_lock_release)(void *context, ani_daemon_bootstrap_lock_t *lock_io);
    bool (*spawn_daemon)(void *context, const ani_daemon_bootstrap_launch_spec_t *spec);
    void (*visible_diagnostic)(void *context, const char *message);
} ani_daemon_bootstrap_ops_t;

ani_daemon_bootstrap_status_t ani_daemon_bootstrap_execute(
    const ani_daemon_bootstrap_config_t *config, ani_daemon_bootstrap_result_t *result_out);

/* Test seam for the same state machine. All callbacks are synchronous and
 * borrowed for the duration of the call. */
ani_daemon_bootstrap_status_t ani_daemon_bootstrap_execute_with_ops(
    const ani_daemon_bootstrap_config_t *config, const ani_daemon_bootstrap_ops_t *ops,
    ani_daemon_bootstrap_result_t *result_out);

#endif /* ANI_DAEMON_BOOTSTRAP_H */
