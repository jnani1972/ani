/*
 * daemon.c — Process-local coordination and wire framing for the ANI daemon.
 */
#include "daemon/daemon.h"

#include "foundation/compat_thread.h"

#include <stdlib.h>
#include <string.h>

typedef struct ani_daemon_subscription {
    ani_daemon_subscription_id_t id;
    ani_daemon_client_id_t client_id;
    struct ani_daemon_subscription *next;
} ani_daemon_subscription_t;

typedef struct ani_daemon_client {
    ani_daemon_client_id_t id;
    uint64_t last_heartbeat_ms;
    struct ani_daemon_client *next;
} ani_daemon_client_t;

typedef struct ani_daemon_job {
    char *project_key;
    ani_daemon_job_state_t state;
    ani_daemon_subscription_t *subscriptions;
    size_t subscription_count;
    bool cancel_callback_inflight;
    bool detached;
    struct ani_daemon_job *next;
    struct ani_daemon_job *action_next;
} ani_daemon_job_t;

typedef struct ani_daemon_watch {
    char *project_key;
    ani_daemon_subscription_t *subscriptions;
    size_t subscription_count;
    struct ani_daemon_watch *next;
    struct ani_daemon_watch *action_next;
} ani_daemon_watch_t;

struct ani_daemon_coordinator {
    ani_mutex_t mutex;
    ani_daemon_client_t *clients;
    ani_daemon_job_t *jobs;
    ani_daemon_watch_t *watches;
    size_t client_count;
    /* See ani_daemon_coordinator_set_permanent. */
    bool permanent;
    size_t job_count;
    size_t watch_count;
    size_t callback_count;
    uint64_t lease_timeout_ms;
    ani_daemon_client_id_t last_client_id;
    ani_daemon_subscription_id_t last_subscription_id;
    ani_daemon_coordinator_state_t state;
    ani_daemon_coordinator_hooks_t hooks;
};

typedef struct {
    ani_daemon_job_t *jobs;
    ani_daemon_watch_t *watches;
    ani_daemon_job_cancel_fn cancel_job;
    ani_daemon_watch_release_fn release_watch;
    void *context;
} ani_daemon_callback_batch_t;

enum {
    FRAME_MAGIC_0 = 0,
    FRAME_MAGIC_1 = 1,
    FRAME_MAGIC_2 = 2,
    FRAME_MAGIC_3 = 3,
    FRAME_VERSION = 4,
    FRAME_TYPE = 5,
    FRAME_FLAGS_HI = 6,
    FRAME_FLAGS_LO = 7,
    FRAME_LENGTH_3 = 8,
    FRAME_LENGTH_2 = 9,
    FRAME_LENGTH_1 = 10,
    FRAME_LENGTH_0 = 11,
};

static bool frame_type_valid(ani_daemon_frame_type_t type) {
    return type == ANI_DAEMON_FRAME_REQUEST || type == ANI_DAEMON_FRAME_RESPONSE;
}

static char *daemon_string_dup(const char *value) {
    size_t length = strlen(value);
    char *copy = malloc(length + 1);
    if (copy) {
        memcpy(copy, value, length + 1);
    }
    return copy;
}

static void free_subscriptions(ani_daemon_subscription_t *subscription) {
    while (subscription) {
        ani_daemon_subscription_t *next = subscription->next;
        free(subscription);
        subscription = next;
    }
}

static void free_job(ani_daemon_job_t *job) {
    if (job) {
        free_subscriptions(job->subscriptions);
        free(job->project_key);
        free(job);
    }
}

static void free_watch(ani_daemon_watch_t *watch) {
    if (watch) {
        free_subscriptions(watch->subscriptions);
        free(watch->project_key);
        free(watch);
    }
}

static ani_daemon_client_id_t issue_client_id_locked(ani_daemon_coordinator_t *coordinator) {
    if (coordinator->last_client_id == UINT64_MAX) {
        return ANI_DAEMON_CLIENT_ID_INVALID;
    }
    coordinator->last_client_id++;
    return coordinator->last_client_id;
}

static ani_daemon_subscription_id_t issue_subscription_id_locked(
    ani_daemon_coordinator_t *coordinator) {
    if (coordinator->last_subscription_id == UINT64_MAX) {
        return ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    }
    coordinator->last_subscription_id++;
    return coordinator->last_subscription_id;
}

static ani_daemon_client_t *find_client_locked(ani_daemon_coordinator_t *coordinator,
                                               ani_daemon_client_id_t client_id) {
    for (ani_daemon_client_t *client = coordinator->clients; client; client = client->next) {
        if (client->id == client_id) {
            return client;
        }
    }
    return NULL;
}

static ani_daemon_job_t *find_job_locked(ani_daemon_coordinator_t *coordinator,
                                         const char *project_key) {
    for (ani_daemon_job_t *job = coordinator->jobs; job; job = job->next) {
        if (strcmp(job->project_key, project_key) == 0) {
            return job;
        }
    }
    return NULL;
}

static ani_daemon_watch_t *find_watch_locked(ani_daemon_coordinator_t *coordinator,
                                             const char *project_key) {
    for (ani_daemon_watch_t *watch = coordinator->watches; watch; watch = watch->next) {
        if (strcmp(watch->project_key, project_key) == 0) {
            return watch;
        }
    }
    return NULL;
}

static bool remove_subscription_locked(ani_daemon_subscription_t **subscriptions,
                                       size_t *subscription_count, ani_daemon_client_id_t client_id,
                                       ani_daemon_subscription_id_t subscription_id) {
    ani_daemon_subscription_t **cursor = subscriptions;
    while (*cursor) {
        ani_daemon_subscription_t *subscription = *cursor;
        if (subscription->id == subscription_id && subscription->client_id == client_id) {
            *cursor = subscription->next;
            free(subscription);
            (*subscription_count)--;
            return true;
        }
        cursor = &subscription->next;
    }
    return false;
}

static void remove_client_subscriptions_locked(ani_daemon_subscription_t **subscriptions,
                                               size_t *subscription_count,
                                               ani_daemon_client_id_t client_id) {
    ani_daemon_subscription_t **cursor = subscriptions;
    while (*cursor) {
        ani_daemon_subscription_t *subscription = *cursor;
        if (subscription->client_id == client_id) {
            *cursor = subscription->next;
            free(subscription);
            (*subscription_count)--;
        } else {
            cursor = &subscription->next;
        }
    }
}

static void callback_batch_init_locked(ani_daemon_coordinator_t *coordinator,
                                       ani_daemon_callback_batch_t *batch) {
    memset(batch, 0, sizeof(*batch));
    batch->cancel_job = coordinator->hooks.cancel_job;
    batch->release_watch = coordinator->hooks.release_watch;
    batch->context = coordinator->hooks.context;
}

static void request_job_cancel_locked(ani_daemon_coordinator_t *coordinator, ani_daemon_job_t *job,
                                      ani_daemon_callback_batch_t *batch) {
    if (job->subscription_count != 0 || job->state != ANI_DAEMON_JOB_RUNNING) {
        return;
    }
    job->state = ANI_DAEMON_JOB_CANCEL_REQUESTED;
    if (batch->cancel_job) {
        job->cancel_callback_inflight = true;
        job->action_next = batch->jobs;
        batch->jobs = job;
        coordinator->callback_count++;
    }
}

static void queue_watch_release_locked(ani_daemon_coordinator_t *coordinator,
                                       ani_daemon_watch_t *watch,
                                       ani_daemon_callback_batch_t *batch) {
    watch->action_next = batch->watches;
    batch->watches = watch;
    if (batch->release_watch) {
        coordinator->callback_count++;
    }
}

static void callback_batch_run(ani_daemon_coordinator_t *coordinator,
                               ani_daemon_callback_batch_t *batch) {
    ani_daemon_job_t *job = batch->jobs;
    while (job) {
        ani_daemon_job_t *next = job->action_next;
        batch->cancel_job(job->project_key, batch->context);

        ani_mutex_lock(&coordinator->mutex);
        coordinator->callback_count--;
        job->cancel_callback_inflight = false;
        bool detached = job->detached;
        ani_mutex_unlock(&coordinator->mutex);
        if (detached) {
            free_job(job);
        }
        job = next;
    }

    ani_daemon_watch_t *watch = batch->watches;
    while (watch) {
        ani_daemon_watch_t *next = watch->action_next;
        if (batch->release_watch) {
            batch->release_watch(watch->project_key, batch->context);
            ani_mutex_lock(&coordinator->mutex);
            coordinator->callback_count--;
            ani_mutex_unlock(&coordinator->mutex);
        }
        free_watch(watch);
        watch = next;
    }
}

static void release_client_resources_locked(ani_daemon_coordinator_t *coordinator,
                                            ani_daemon_client_id_t client_id,
                                            ani_daemon_callback_batch_t *batch) {
    for (ani_daemon_job_t *job = coordinator->jobs; job; job = job->next) {
        remove_client_subscriptions_locked(&job->subscriptions, &job->subscription_count,
                                           client_id);
        request_job_cancel_locked(coordinator, job, batch);
    }

    ani_daemon_watch_t **watch_cursor = &coordinator->watches;
    while (*watch_cursor) {
        ani_daemon_watch_t *watch = *watch_cursor;
        remove_client_subscriptions_locked(&watch->subscriptions, &watch->subscription_count,
                                           client_id);
        if (watch->subscription_count == 0) {
            *watch_cursor = watch->next;
            watch->next = NULL;
            coordinator->watch_count--;
            queue_watch_release_locked(coordinator, watch, batch);
        } else {
            watch_cursor = &watch->next;
        }
    }
}

static void release_client_locked(ani_daemon_coordinator_t *coordinator,
                                  ani_daemon_client_t *client, ani_daemon_callback_batch_t *batch) {
    release_client_resources_locked(coordinator, client->id, batch);
    free(client);
    coordinator->client_count--;
    if (coordinator->client_count == 0 && !coordinator->permanent) {
        coordinator->state = ANI_DAEMON_COORDINATOR_STOPPING;
    }
}

static bool terminal_job_locked(ani_daemon_coordinator_t *coordinator, const char *project_key,
                                bool require_cancellation, ani_daemon_job_t **free_after_unlock) {
    ani_daemon_job_t **cursor = &coordinator->jobs;
    while (*cursor && strcmp((*cursor)->project_key, project_key) != 0) {
        cursor = &(*cursor)->next;
    }
    if (!*cursor || (require_cancellation && (*cursor)->state == ANI_DAEMON_JOB_RUNNING)) {
        return false;
    }

    ani_daemon_job_t *job = *cursor;
    *cursor = job->next;
    job->next = NULL;
    job->detached = true;
    coordinator->job_count--;
    free_subscriptions(job->subscriptions);
    job->subscriptions = NULL;
    job->subscription_count = 0;
    if (!job->cancel_callback_inflight) {
        *free_after_unlock = job;
    }
    return true;
}

void ani_daemon_coordinator_set_permanent(ani_daemon_coordinator_t *coordinator, bool permanent) {
    if (!coordinator) {
        return;
    }
    ani_mutex_lock(&coordinator->mutex);
    coordinator->permanent = permanent;
    ani_mutex_unlock(&coordinator->mutex);
}

ani_daemon_coordinator_t *ani_daemon_coordinator_new(uint64_t lease_timeout_ms) {
    ani_daemon_coordinator_t *coordinator = calloc(1, sizeof(*coordinator));
    if (!coordinator) {
        return NULL;
    }
    ani_mutex_init(&coordinator->mutex);
    coordinator->lease_timeout_ms = lease_timeout_ms;
    coordinator->state = ANI_DAEMON_COORDINATOR_RUNNING;
    return coordinator;
}

void ani_daemon_coordinator_free(ani_daemon_coordinator_t *coordinator) {
    if (!coordinator) {
        return;
    }

    ani_daemon_client_t *client = coordinator->clients;
    while (client) {
        ani_daemon_client_t *next = client->next;
        free(client);
        client = next;
    }

    ani_daemon_job_t *job = coordinator->jobs;
    while (job) {
        ani_daemon_job_t *next = job->next;
        free_job(job);
        job = next;
    }

    ani_daemon_watch_t *watch = coordinator->watches;
    while (watch) {
        ani_daemon_watch_t *next = watch->next;
        free_watch(watch);
        watch = next;
    }
    ani_mutex_destroy(&coordinator->mutex);
    free(coordinator);
}

bool ani_daemon_coordinator_set_hooks(ani_daemon_coordinator_t *coordinator,
                                      const ani_daemon_coordinator_hooks_t *hooks) {
    if (!coordinator || !hooks) {
        return false;
    }
    ani_mutex_lock(&coordinator->mutex);
    coordinator->hooks = *hooks;
    ani_mutex_unlock(&coordinator->mutex);
    return true;
}

ani_daemon_coordinator_state_t ani_daemon_coordinator_state(ani_daemon_coordinator_t *coordinator) {
    if (!coordinator) {
        return ANI_DAEMON_COORDINATOR_STOPPING;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_coordinator_state_t state = coordinator->state;
    ani_mutex_unlock(&coordinator->mutex);
    return state;
}

ani_daemon_client_id_t ani_daemon_client_connected(ani_daemon_coordinator_t *coordinator,
                                                   uint64_t now_ms) {
    if (!coordinator) {
        return ANI_DAEMON_CLIENT_ID_INVALID;
    }

    ani_daemon_client_t *client = malloc(sizeof(*client));
    if (!client) {
        return ANI_DAEMON_CLIENT_ID_INVALID;
    }

    ani_mutex_lock(&coordinator->mutex);
    if (coordinator->state != ANI_DAEMON_COORDINATOR_RUNNING) {
        ani_mutex_unlock(&coordinator->mutex);
        free(client);
        return ANI_DAEMON_CLIENT_ID_INVALID;
    }
    ani_daemon_client_id_t client_id = issue_client_id_locked(coordinator);
    if (client_id == ANI_DAEMON_CLIENT_ID_INVALID) {
        ani_mutex_unlock(&coordinator->mutex);
        free(client);
        return ANI_DAEMON_CLIENT_ID_INVALID;
    }
    client->id = client_id;
    client->last_heartbeat_ms = now_ms;
    client->next = coordinator->clients;
    coordinator->clients = client;
    coordinator->client_count++;
    ani_mutex_unlock(&coordinator->mutex);
    return client_id;
}

bool ani_daemon_client_disconnected(ani_daemon_coordinator_t *coordinator,
                                    ani_daemon_client_id_t client_id, uint64_t now_ms) {
    (void)now_ms;
    if (!coordinator || client_id == ANI_DAEMON_CLIENT_ID_INVALID) {
        return false;
    }

    ani_daemon_callback_batch_t batch;
    ani_mutex_lock(&coordinator->mutex);
    callback_batch_init_locked(coordinator, &batch);
    ani_daemon_client_t **cursor = &coordinator->clients;
    while (*cursor && (*cursor)->id != client_id) {
        cursor = &(*cursor)->next;
    }
    if (!*cursor) {
        ani_mutex_unlock(&coordinator->mutex);
        return false;
    }
    ani_daemon_client_t *client = *cursor;
    *cursor = client->next;
    release_client_locked(coordinator, client, &batch);
    ani_mutex_unlock(&coordinator->mutex);

    callback_batch_run(coordinator, &batch);
    return true;
}

bool ani_daemon_client_heartbeat(ani_daemon_coordinator_t *coordinator,
                                 ani_daemon_client_id_t client_id, uint64_t now_ms) {
    if (!coordinator || client_id == ANI_DAEMON_CLIENT_ID_INVALID) {
        return false;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_client_t *client = find_client_locked(coordinator, client_id);
    bool found = client != NULL;
    if (client && now_ms > client->last_heartbeat_ms) {
        client->last_heartbeat_ms = now_ms;
    }
    ani_mutex_unlock(&coordinator->mutex);
    return found;
}

size_t ani_daemon_expire_leases(ani_daemon_coordinator_t *coordinator, uint64_t now_ms) {
    if (!coordinator) {
        return 0;
    }

    size_t expired_count = 0;
    ani_daemon_callback_batch_t batch;
    ani_mutex_lock(&coordinator->mutex);
    callback_batch_init_locked(coordinator, &batch);
    ani_daemon_client_t **cursor = &coordinator->clients;
    while (*cursor) {
        ani_daemon_client_t *client = *cursor;
        bool expired = now_ms >= client->last_heartbeat_ms &&
                       now_ms - client->last_heartbeat_ms >= coordinator->lease_timeout_ms;
        if (!expired) {
            cursor = &client->next;
            continue;
        }
        *cursor = client->next;
        release_client_locked(coordinator, client, &batch);
        expired_count++;
    }
    ani_mutex_unlock(&coordinator->mutex);

    callback_batch_run(coordinator, &batch);
    return expired_count;
}

size_t ani_daemon_active_clients(ani_daemon_coordinator_t *coordinator) {
    if (!coordinator) {
        return 0;
    }
    ani_mutex_lock(&coordinator->mutex);
    size_t count = coordinator->client_count;
    ani_mutex_unlock(&coordinator->mutex);
    return count;
}

ani_daemon_subscription_result_t ani_daemon_job_subscribe(
    ani_daemon_coordinator_t *coordinator, ani_daemon_client_id_t client_id,
    const char *project_key, ani_daemon_subscription_id_t *subscription_id) {
    if (subscription_id) {
        *subscription_id = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    }
    if (!coordinator || !subscription_id || !project_key || project_key[0] == '\0' ||
        client_id == ANI_DAEMON_CLIENT_ID_INVALID) {
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }

    ani_mutex_lock(&coordinator->mutex);
    if (coordinator->state != ANI_DAEMON_COORDINATOR_RUNNING ||
        !find_client_locked(coordinator, client_id)) {
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }
    ani_daemon_job_t *job = find_job_locked(coordinator, project_key);
    if (job && job->state != ANI_DAEMON_JOB_RUNNING) {
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }

    bool started = job == NULL;
    ani_daemon_job_t *new_job = NULL;
    char *key_copy = NULL;
    ani_daemon_subscription_t *subscription = malloc(sizeof(*subscription));
    if (started) {
        new_job = calloc(1, sizeof(*new_job));
        key_copy = daemon_string_dup(project_key);
    }
    if (!subscription || (started && (!new_job || !key_copy))) {
        free(subscription);
        free(new_job);
        free(key_copy);
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }

    ani_daemon_subscription_id_t id = issue_subscription_id_locked(coordinator);
    if (id == ANI_DAEMON_SUBSCRIPTION_ID_INVALID) {
        free(subscription);
        free(new_job);
        free(key_copy);
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }
    if (started) {
        new_job->project_key = key_copy;
        new_job->state = ANI_DAEMON_JOB_RUNNING;
        new_job->next = coordinator->jobs;
        coordinator->jobs = new_job;
        coordinator->job_count++;
        job = new_job;
    }
    subscription->id = id;
    subscription->client_id = client_id;
    subscription->next = job->subscriptions;
    job->subscriptions = subscription;
    job->subscription_count++;
    *subscription_id = id;
    ani_mutex_unlock(&coordinator->mutex);
    return started ? ANI_DAEMON_SUBSCRIPTION_STARTED : ANI_DAEMON_SUBSCRIPTION_JOINED;
}

ani_daemon_subscription_result_t ani_daemon_watch_subscribe(
    ani_daemon_coordinator_t *coordinator, ani_daemon_client_id_t client_id,
    const char *project_key, ani_daemon_subscription_id_t *subscription_id) {
    if (subscription_id) {
        *subscription_id = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    }
    if (!coordinator || !subscription_id || !project_key || project_key[0] == '\0' ||
        client_id == ANI_DAEMON_CLIENT_ID_INVALID) {
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }

    ani_mutex_lock(&coordinator->mutex);
    if (coordinator->state != ANI_DAEMON_COORDINATOR_RUNNING ||
        !find_client_locked(coordinator, client_id)) {
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }
    ani_daemon_watch_t *watch = find_watch_locked(coordinator, project_key);
    bool started = watch == NULL;
    ani_daemon_watch_t *new_watch = NULL;
    char *key_copy = NULL;
    ani_daemon_subscription_t *subscription = malloc(sizeof(*subscription));
    if (started) {
        new_watch = calloc(1, sizeof(*new_watch));
        key_copy = daemon_string_dup(project_key);
    }
    if (!subscription || (started && (!new_watch || !key_copy))) {
        free(subscription);
        free(new_watch);
        free(key_copy);
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }

    ani_daemon_subscription_id_t id = issue_subscription_id_locked(coordinator);
    if (id == ANI_DAEMON_SUBSCRIPTION_ID_INVALID) {
        free(subscription);
        free(new_watch);
        free(key_copy);
        ani_mutex_unlock(&coordinator->mutex);
        return ANI_DAEMON_SUBSCRIPTION_REJECTED;
    }
    if (started) {
        new_watch->project_key = key_copy;
        new_watch->next = coordinator->watches;
        coordinator->watches = new_watch;
        coordinator->watch_count++;
        watch = new_watch;
    }
    subscription->id = id;
    subscription->client_id = client_id;
    subscription->next = watch->subscriptions;
    watch->subscriptions = subscription;
    watch->subscription_count++;
    *subscription_id = id;
    ani_mutex_unlock(&coordinator->mutex);
    return started ? ANI_DAEMON_SUBSCRIPTION_STARTED : ANI_DAEMON_SUBSCRIPTION_JOINED;
}

bool ani_daemon_job_unsubscribe(ani_daemon_coordinator_t *coordinator,
                                ani_daemon_client_id_t client_id,
                                ani_daemon_subscription_id_t subscription_id) {
    if (!coordinator || client_id == ANI_DAEMON_CLIENT_ID_INVALID ||
        subscription_id == ANI_DAEMON_SUBSCRIPTION_ID_INVALID) {
        return false;
    }

    ani_daemon_callback_batch_t batch;
    ani_mutex_lock(&coordinator->mutex);
    callback_batch_init_locked(coordinator, &batch);
    if (!find_client_locked(coordinator, client_id)) {
        ani_mutex_unlock(&coordinator->mutex);
        return false;
    }
    bool removed = false;
    for (ani_daemon_job_t *job = coordinator->jobs; job; job = job->next) {
        removed = remove_subscription_locked(&job->subscriptions, &job->subscription_count,
                                             client_id, subscription_id);
        if (removed) {
            request_job_cancel_locked(coordinator, job, &batch);
            break;
        }
    }
    ani_mutex_unlock(&coordinator->mutex);
    callback_batch_run(coordinator, &batch);
    return removed;
}

bool ani_daemon_watch_unsubscribe(ani_daemon_coordinator_t *coordinator,
                                  ani_daemon_client_id_t client_id,
                                  ani_daemon_subscription_id_t subscription_id) {
    if (!coordinator || client_id == ANI_DAEMON_CLIENT_ID_INVALID ||
        subscription_id == ANI_DAEMON_SUBSCRIPTION_ID_INVALID) {
        return false;
    }

    ani_daemon_callback_batch_t batch;
    ani_mutex_lock(&coordinator->mutex);
    callback_batch_init_locked(coordinator, &batch);
    if (!find_client_locked(coordinator, client_id)) {
        ani_mutex_unlock(&coordinator->mutex);
        return false;
    }
    bool removed = false;
    ani_daemon_watch_t **cursor = &coordinator->watches;
    while (*cursor) {
        ani_daemon_watch_t *watch = *cursor;
        removed = remove_subscription_locked(&watch->subscriptions, &watch->subscription_count,
                                             client_id, subscription_id);
        if (!removed) {
            cursor = &watch->next;
            continue;
        }
        if (watch->subscription_count == 0) {
            *cursor = watch->next;
            watch->next = NULL;
            coordinator->watch_count--;
            queue_watch_release_locked(coordinator, watch, &batch);
        }
        break;
    }
    ani_mutex_unlock(&coordinator->mutex);
    callback_batch_run(coordinator, &batch);
    return removed;
}

size_t ani_daemon_job_subscribers(ani_daemon_coordinator_t *coordinator, const char *project_key) {
    if (!coordinator || !project_key) {
        return 0;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_job_t *job = find_job_locked(coordinator, project_key);
    size_t count = job ? job->subscription_count : 0;
    ani_mutex_unlock(&coordinator->mutex);
    return count;
}

size_t ani_daemon_watch_subscribers(ani_daemon_coordinator_t *coordinator,
                                    const char *project_key) {
    if (!coordinator || !project_key) {
        return 0;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_watch_t *watch = find_watch_locked(coordinator, project_key);
    size_t count = watch ? watch->subscription_count : 0;
    ani_mutex_unlock(&coordinator->mutex);
    return count;
}

size_t ani_daemon_active_jobs(ani_daemon_coordinator_t *coordinator) {
    if (!coordinator) {
        return 0;
    }
    ani_mutex_lock(&coordinator->mutex);
    size_t count = coordinator->job_count;
    ani_mutex_unlock(&coordinator->mutex);
    return count;
}

size_t ani_daemon_active_watches(ani_daemon_coordinator_t *coordinator) {
    if (!coordinator) {
        return 0;
    }
    ani_mutex_lock(&coordinator->mutex);
    size_t count = coordinator->watch_count;
    ani_mutex_unlock(&coordinator->mutex);
    return count;
}

ani_daemon_job_state_t ani_daemon_job_state(ani_daemon_coordinator_t *coordinator,
                                            const char *project_key) {
    if (!coordinator || !project_key) {
        return ANI_DAEMON_JOB_NONE;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_job_t *job = find_job_locked(coordinator, project_key);
    ani_daemon_job_state_t state = job ? job->state : ANI_DAEMON_JOB_NONE;
    ani_mutex_unlock(&coordinator->mutex);
    return state;
}

bool ani_daemon_job_reaping(ani_daemon_coordinator_t *coordinator, const char *project_key) {
    if (!coordinator || !project_key) {
        return false;
    }
    ani_mutex_lock(&coordinator->mutex);
    ani_daemon_job_t *job = find_job_locked(coordinator, project_key);
    bool transitioned = job && job->state == ANI_DAEMON_JOB_CANCEL_REQUESTED;
    if (transitioned) {
        job->state = ANI_DAEMON_JOB_REAPING;
    }
    ani_mutex_unlock(&coordinator->mutex);
    return transitioned;
}

bool ani_daemon_job_reaped(ani_daemon_coordinator_t *coordinator, const char *project_key,
                           uint64_t now_ms) {
    (void)now_ms;
    if (!coordinator || !project_key) {
        return false;
    }
    ani_daemon_job_t *free_after_unlock = NULL;
    ani_mutex_lock(&coordinator->mutex);
    bool removed = terminal_job_locked(coordinator, project_key, true, &free_after_unlock);
    ani_mutex_unlock(&coordinator->mutex);
    free_job(free_after_unlock);
    return removed;
}

bool ani_daemon_job_completed(ani_daemon_coordinator_t *coordinator, const char *project_key,
                              uint64_t now_ms) {
    (void)now_ms;
    if (!coordinator || !project_key) {
        return false;
    }
    ani_daemon_job_t *free_after_unlock = NULL;
    ani_mutex_lock(&coordinator->mutex);
    bool removed = terminal_job_locked(coordinator, project_key, false, &free_after_unlock);
    ani_mutex_unlock(&coordinator->mutex);
    free_job(free_after_unlock);
    return removed;
}

bool ani_daemon_should_exit(ani_daemon_coordinator_t *coordinator, uint64_t now_ms) {
    (void)now_ms;
    if (!coordinator) {
        return false;
    }
    ani_mutex_lock(&coordinator->mutex);
    bool should_exit = coordinator->state == ANI_DAEMON_COORDINATOR_STOPPING &&
                       coordinator->client_count == 0 && coordinator->job_count == 0 &&
                       coordinator->watch_count == 0 && coordinator->callback_count == 0;
    ani_mutex_unlock(&coordinator->mutex);
    return should_exit;
}

bool ani_daemon_frame_header_encode(uint8_t header[ANI_DAEMON_FRAME_HEADER_SIZE],
                                    ani_daemon_frame_type_t type, uint16_t flags, uint32_t length) {
    if (!header || !frame_type_valid(type) || length > ANI_DAEMON_MAX_FRAME_SIZE) {
        return false;
    }
    header[FRAME_MAGIC_0] = 'C';
    header[FRAME_MAGIC_1] = 'B';
    header[FRAME_MAGIC_2] = 'M';
    header[FRAME_MAGIC_3] = 'D';
    header[FRAME_VERSION] = ANI_DAEMON_RENDEZVOUS_FRAME_VERSION;
    header[FRAME_TYPE] = (uint8_t)type;
    header[FRAME_FLAGS_HI] = (uint8_t)(flags >> 8);
    header[FRAME_FLAGS_LO] = (uint8_t)flags;
    header[FRAME_LENGTH_3] = (uint8_t)(length >> 24);
    header[FRAME_LENGTH_2] = (uint8_t)(length >> 16);
    header[FRAME_LENGTH_1] = (uint8_t)(length >> 8);
    header[FRAME_LENGTH_0] = (uint8_t)length;
    return true;
}

bool ani_daemon_frame_header_decode(const uint8_t header[ANI_DAEMON_FRAME_HEADER_SIZE],
                                    ani_daemon_frame_t *frame) {
    if (!header || !frame || header[FRAME_MAGIC_0] != 'C' || header[FRAME_MAGIC_1] != 'B' ||
        header[FRAME_MAGIC_2] != 'M' || header[FRAME_MAGIC_3] != 'D' ||
        header[FRAME_VERSION] != ANI_DAEMON_RENDEZVOUS_FRAME_VERSION) {
        return false;
    }

    ani_daemon_frame_type_t type = (ani_daemon_frame_type_t)header[FRAME_TYPE];
    uint32_t length = ((uint32_t)header[FRAME_LENGTH_3] << 24) |
                      ((uint32_t)header[FRAME_LENGTH_2] << 16) |
                      ((uint32_t)header[FRAME_LENGTH_1] << 8) | (uint32_t)header[FRAME_LENGTH_0];
    if (!frame_type_valid(type) || length > ANI_DAEMON_MAX_FRAME_SIZE) {
        return false;
    }

    frame->type = type;
    frame->flags =
        (uint16_t)(((uint16_t)header[FRAME_FLAGS_HI] << 8) | (uint16_t)header[FRAME_FLAGS_LO]);
    frame->length = length;
    return true;
}
