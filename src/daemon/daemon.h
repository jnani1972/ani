/*
 * daemon.h — Process-local coordination and wire framing for the ANI daemon.
 *
 * Transport and worker supervision live outside this module. The coordinator
 * binds clients and resource subscriptions to transport connections, coalesces
 * shared work, and defines the daemon's terminal shutdown transition.
 */
#ifndef ANI_DAEMON_H
#define ANI_DAEMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Permanent framing version for the account-wide rendezvous endpoint. Never
 * bump this for detailed runtime payload changes: incompatible executable
 * generations must still exchange the stable HELLO conflict envelope. */
#define ANI_DAEMON_RENDEZVOUS_FRAME_VERSION 1U
#define ANI_DAEMON_FRAME_HEADER_SIZE 12U
#define ANI_DAEMON_MAX_FRAME_SIZE (10U * 1024U * 1024U)
#define ANI_DAEMON_KEY_SIZE 17U

typedef enum {
    ANI_DAEMON_FRAME_REQUEST = 1,
    ANI_DAEMON_FRAME_RESPONSE = 2,
} ani_daemon_frame_type_t;

typedef struct {
    ani_daemon_frame_type_t type;
    uint16_t flags;
    uint32_t length;
} ani_daemon_frame_t;

typedef struct ani_daemon_coordinator ani_daemon_coordinator_t;

typedef uint64_t ani_daemon_client_id_t;
typedef uint64_t ani_daemon_subscription_id_t;

#define ANI_DAEMON_CLIENT_ID_INVALID ((ani_daemon_client_id_t)0)
#define ANI_DAEMON_SUBSCRIPTION_ID_INVALID ((ani_daemon_subscription_id_t)0)

typedef enum {
    ANI_DAEMON_COORDINATOR_RUNNING = 1,
    ANI_DAEMON_COORDINATOR_STOPPING = 2,
} ani_daemon_coordinator_state_t;

typedef enum {
    ANI_DAEMON_SUBSCRIPTION_REJECTED = 0,
    ANI_DAEMON_SUBSCRIPTION_STARTED = 1,
    ANI_DAEMON_SUBSCRIPTION_JOINED = 2,
} ani_daemon_subscription_result_t;

typedef enum {
    ANI_DAEMON_JOB_NONE = 0,
    ANI_DAEMON_JOB_RUNNING = 1,
    ANI_DAEMON_JOB_CANCEL_REQUESTED = 2,
    ANI_DAEMON_JOB_REAPING = 3,
} ani_daemon_job_state_t;

typedef void (*ani_daemon_job_cancel_fn)(const char *project_key, void *context);
typedef void (*ani_daemon_watch_release_fn)(const char *project_key, void *context);

typedef struct {
    ani_daemon_job_cancel_fn cancel_job;
    ani_daemon_watch_release_fn release_watch;
    void *context;
} ani_daemon_coordinator_hooks_t;

/* lease_timeout_ms is fixed for the coordinator lifetime. All timestamps must
 * come from the same monotonic clock domain. */
ani_daemon_coordinator_t *ani_daemon_coordinator_new(uint64_t lease_timeout_ms);

/* A PERMANENT coordinator (backing a `daemon start` generation) never
 * self-transitions to STOPPING when its client count reaches zero; only the
 * explicit stop/drain paths end it. */
void ani_daemon_coordinator_set_permanent(ani_daemon_coordinator_t *coordinator, bool permanent);
/* The caller must first quiesce coordinator calls and hook invocations. */
void ani_daemon_coordinator_free(ani_daemon_coordinator_t *coordinator);

/* Hooks are copied. Their context must remain valid until the coordinator is
 * quiescent. Hooks are always invoked after releasing the coordinator mutex. */
bool ani_daemon_coordinator_set_hooks(ani_daemon_coordinator_t *coordinator,
                                      const ani_daemon_coordinator_hooks_t *hooks);
ani_daemon_coordinator_state_t ani_daemon_coordinator_state(ani_daemon_coordinator_t *coordinator);

/* Client IDs are daemon-issued, nonzero, monotonic, and never recycled. */
ani_daemon_client_id_t ani_daemon_client_connected(ani_daemon_coordinator_t *coordinator,
                                                   uint64_t now_ms);
bool ani_daemon_client_disconnected(ani_daemon_coordinator_t *coordinator,
                                    ani_daemon_client_id_t client_id, uint64_t now_ms);
bool ani_daemon_client_heartbeat(ani_daemon_coordinator_t *coordinator,
                                 ani_daemon_client_id_t client_id, uint64_t now_ms);
size_t ani_daemon_expire_leases(ani_daemon_coordinator_t *coordinator, uint64_t now_ms);
size_t ani_daemon_active_clients(ani_daemon_coordinator_t *coordinator);

/* Every accepted subscription receives a unique daemon-issued handle. The
 * first subscriber starts the physical resource; later subscribers join it. */
ani_daemon_subscription_result_t ani_daemon_job_subscribe(
    ani_daemon_coordinator_t *coordinator, ani_daemon_client_id_t client_id,
    const char *project_key, ani_daemon_subscription_id_t *subscription_id);
ani_daemon_subscription_result_t ani_daemon_watch_subscribe(
    ani_daemon_coordinator_t *coordinator, ani_daemon_client_id_t client_id,
    const char *project_key, ani_daemon_subscription_id_t *subscription_id);
bool ani_daemon_job_unsubscribe(ani_daemon_coordinator_t *coordinator,
                                ani_daemon_client_id_t client_id,
                                ani_daemon_subscription_id_t subscription_id);
bool ani_daemon_watch_unsubscribe(ani_daemon_coordinator_t *coordinator,
                                  ani_daemon_client_id_t client_id,
                                  ani_daemon_subscription_id_t subscription_id);

size_t ani_daemon_job_subscribers(ani_daemon_coordinator_t *coordinator, const char *project_key);
size_t ani_daemon_watch_subscribers(ani_daemon_coordinator_t *coordinator, const char *project_key);
size_t ani_daemon_active_jobs(ani_daemon_coordinator_t *coordinator);
size_t ani_daemon_active_watches(ani_daemon_coordinator_t *coordinator);
ani_daemon_job_state_t ani_daemon_job_state(ani_daemon_coordinator_t *coordinator,
                                            const char *project_key);

/* Cancellation is two phase. Losing the final subscriber requests cancel;
 * the job remains active until its supervisor reports completion/reaping. */
bool ani_daemon_job_reaping(ani_daemon_coordinator_t *coordinator, const char *project_key);
bool ani_daemon_job_reaped(ani_daemon_coordinator_t *coordinator, const char *project_key,
                           uint64_t now_ms);
bool ani_daemon_job_completed(ani_daemon_coordinator_t *coordinator, const char *project_key,
                              uint64_t now_ms);

/* STOPPING is terminal. Exit is ready only after every job/watch is gone. */
bool ani_daemon_should_exit(ani_daemon_coordinator_t *coordinator, uint64_t now_ms);

/* Encode/decode the permanently stable 12-byte "ANID" rendezvous frame header
 * in network byte order. Detailed operation ABIs live above this framing. */
bool ani_daemon_frame_header_encode(uint8_t header[ANI_DAEMON_FRAME_HEADER_SIZE],
                                    ani_daemon_frame_type_t type, uint16_t flags, uint32_t length);
bool ani_daemon_frame_header_decode(const uint8_t header[ANI_DAEMON_FRAME_HEADER_SIZE],
                                    ani_daemon_frame_t *frame);

#endif /* ANI_DAEMON_H */
