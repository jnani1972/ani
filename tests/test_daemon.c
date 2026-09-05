/*
 * test_daemon.c — Guards for shared MCP-daemon coordination and framing.
 */
#include "test_framework.h"

#include "daemon/daemon.h"
#include "mcp/mcp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t job_cancel_count;
    size_t shared_job_cancel_count;
    size_t lease_job_cancel_count;
    size_t shutdown_job_cancel_count;
    size_t watch_release_count;
    size_t explicit_watch_release_count;
    size_t lease_watch_release_count;
    size_t shutdown_watch_release_count;
    size_t unexpected_key_count;
} daemon_release_tracker_t;

static void record_job_cancel(const char *project_key, void *context) {
    daemon_release_tracker_t *tracker = context;
    tracker->job_cancel_count++;
    if (strcmp(project_key, "project-shared") == 0) {
        tracker->shared_job_cancel_count++;
    } else if (strcmp(project_key, "project-lease-job") == 0) {
        tracker->lease_job_cancel_count++;
    } else if (strcmp(project_key, "project-shutdown-job") == 0) {
        tracker->shutdown_job_cancel_count++;
    } else {
        tracker->unexpected_key_count++;
    }
}

static void record_watch_release(const char *project_key, void *context) {
    daemon_release_tracker_t *tracker = context;
    tracker->watch_release_count++;
    if (strcmp(project_key, "project-explicit-watch") == 0) {
        tracker->explicit_watch_release_count++;
    } else if (strcmp(project_key, "project-lease-watch") == 0) {
        tracker->lease_watch_release_count++;
    } else if (strcmp(project_key, "project-shutdown-watch") == 0) {
        tracker->shutdown_watch_release_count++;
    } else {
        tracker->unexpected_key_count++;
    }
}

static bool install_release_tracker(ani_daemon_coordinator_t *coordinator,
                                    daemon_release_tracker_t *tracker) {
    const ani_daemon_coordinator_hooks_t hooks = {
        .cancel_job = record_job_cancel,
        .release_watch = record_watch_release,
        .context = tracker,
    };
    return ani_daemon_coordinator_set_hooks(coordinator, &hooks);
}

TEST(daemon_client_ids_are_connection_bound) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    ASSERT_NOT_NULL(c);

    ani_daemon_client_id_t a = ani_daemon_client_connected(c, 1000);
    ani_daemon_client_id_t b = ani_daemon_client_connected(c, 1001);
    ASSERT_NEQ(a, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_NEQ(b, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_NEQ(a, b);
    ASSERT_EQ(ani_daemon_active_clients(c), 2);

    ani_daemon_subscription_id_t rejected = UINT64_MAX;
    ASSERT_EQ(ani_daemon_job_subscribe(c, UINT64_MAX, "project-a", &rejected),
              ANI_DAEMON_SUBSCRIPTION_REJECTED);
    ASSERT_EQ(rejected, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_TRUE(ani_daemon_client_disconnected(c, a, 1010));
    ASSERT_FALSE(ani_daemon_client_disconnected(c, a, 1011));
    rejected = UINT64_MAX;
    ASSERT_EQ(ani_daemon_job_subscribe(c, a, "project-a", &rejected),
              ANI_DAEMON_SUBSCRIPTION_REJECTED);
    ASSERT_EQ(rejected, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);

    ani_daemon_client_id_t c_id = ani_daemon_client_connected(c, 1012);
    ASSERT_NEQ(c_id, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_NEQ(c_id, a); /* closed connection IDs are never recycled */
    ASSERT_NEQ(c_id, b);
    ASSERT_EQ(ani_daemon_active_clients(c), 2);

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_shared_job_survives_until_final_subscriber_disconnects) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    daemon_release_tracker_t tracker = {0};
    ASSERT_NOT_NULL(c);
    ASSERT_TRUE(install_release_tracker(c, &tracker));

    ani_daemon_client_id_t a = ani_daemon_client_connected(c, 2000);
    ani_daemon_client_id_t b = ani_daemon_client_connected(c, 2000);
    ani_daemon_client_id_t observer = ani_daemon_client_connected(c, 2000);
    ASSERT_NEQ(a, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_NEQ(b, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_NEQ(observer, ANI_DAEMON_CLIENT_ID_INVALID);

    ani_daemon_subscription_id_t a_first = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ani_daemon_subscription_id_t a_second = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ani_daemon_subscription_id_t b_subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ASSERT_EQ(ani_daemon_job_subscribe(c, a, "project-shared", &a_first),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_EQ(ani_daemon_job_subscribe(c, a, "project-shared", &a_second),
              ANI_DAEMON_SUBSCRIPTION_JOINED);
    ASSERT_EQ(ani_daemon_job_subscribe(c, b, "project-shared", &b_subscription),
              ANI_DAEMON_SUBSCRIPTION_JOINED);
    ASSERT_NEQ(a_first, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(a_second, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(b_subscription, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(a_first, a_second);
    ASSERT_NEQ(a_first, b_subscription);
    ASSERT_NEQ(a_second, b_subscription);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_job_state(c, "project-shared"), ANI_DAEMON_JOB_RUNNING);
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shared"), 3);

    ASSERT_FALSE(ani_daemon_job_unsubscribe(c, b, a_first));
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shared"), 3);
    ASSERT_TRUE(ani_daemon_job_unsubscribe(c, a, a_first));
    ASSERT_FALSE(ani_daemon_job_unsubscribe(c, a, a_first));
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shared"), 2);
    ASSERT_EQ(tracker.job_cancel_count, 0);

    ASSERT_TRUE(ani_daemon_client_disconnected(c, a, 2001));
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shared"), 1);
    ASSERT_EQ(tracker.job_cancel_count, 0);

    /* The final subscription requests cancellation even while the daemon
     * remains RUNNING because unrelated clients are still connected. */
    ASSERT_TRUE(ani_daemon_job_unsubscribe(c, b, b_subscription));
    ASSERT_FALSE(ani_daemon_job_unsubscribe(c, b, b_subscription));
    ASSERT_EQ(ani_daemon_coordinator_state(c), ANI_DAEMON_COORDINATOR_RUNNING);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shared"), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-shared"), ANI_DAEMON_JOB_CANCEL_REQUESTED);
    ASSERT_EQ(tracker.job_cancel_count, 1);
    ASSERT_EQ(tracker.shared_job_cancel_count, 1);
    ASSERT_EQ(tracker.unexpected_key_count, 0);

    /* Cancellation is asynchronous: the job remains active until its worker
     * has actually been reaped, and duplicate owner cleanup cannot re-fire it. */
    ASSERT_TRUE(ani_daemon_client_disconnected(c, b, 2002));
    ASSERT_EQ(tracker.job_cancel_count, 1);
    ASSERT_EQ(ani_daemon_active_clients(c), 1);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_TRUE(ani_daemon_job_reaped(c, "project-shared", 2003));
    ASSERT_FALSE(ani_daemon_job_reaped(c, "project-shared", 2004));
    ASSERT_EQ(ani_daemon_active_jobs(c), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-shared"), ANI_DAEMON_JOB_NONE);

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_watch_subscription_ids_are_connection_bound) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    daemon_release_tracker_t tracker = {0};
    ASSERT_NOT_NULL(c);
    ASSERT_TRUE(install_release_tracker(c, &tracker));

    ani_daemon_client_id_t a = ani_daemon_client_connected(c, 3000);
    ani_daemon_client_id_t b = ani_daemon_client_connected(c, 3000);
    ani_daemon_subscription_id_t a_watch = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ani_daemon_subscription_id_t b_watch = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ASSERT_EQ(ani_daemon_watch_subscribe(c, a, "project-explicit-watch", &a_watch),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_EQ(ani_daemon_watch_subscribe(c, b, "project-explicit-watch", &b_watch),
              ANI_DAEMON_SUBSCRIPTION_JOINED);
    ASSERT_NEQ(a_watch, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(b_watch, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(a_watch, b_watch);
    ASSERT_EQ(ani_daemon_watch_subscribers(c, "project-explicit-watch"), 2);

    ASSERT_FALSE(ani_daemon_watch_unsubscribe(c, b, a_watch));
    ASSERT_TRUE(ani_daemon_watch_unsubscribe(c, a, a_watch));
    ASSERT_FALSE(ani_daemon_watch_unsubscribe(c, a, a_watch));
    ASSERT_EQ(ani_daemon_active_watches(c), 1);
    ASSERT_EQ(ani_daemon_watch_subscribers(c, "project-explicit-watch"), 1);
    ASSERT_EQ(tracker.watch_release_count, 0);

    ASSERT_TRUE(ani_daemon_watch_unsubscribe(c, b, b_watch));
    ASSERT_FALSE(ani_daemon_watch_unsubscribe(c, b, b_watch));
    ASSERT_EQ(ani_daemon_active_watches(c), 0);
    ASSERT_EQ(ani_daemon_watch_subscribers(c, "project-explicit-watch"), 0);
    ASSERT_EQ(tracker.watch_release_count, 1);
    ASSERT_EQ(tracker.explicit_watch_release_count, 1);
    ASSERT_EQ(tracker.unexpected_key_count, 0);

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_heartbeat_extends_lease_and_expiry_releases_connection) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    daemon_release_tracker_t tracker = {0};
    ASSERT_NOT_NULL(c);
    ASSERT_TRUE(install_release_tracker(c, &tracker));

    ani_daemon_client_id_t owner = ani_daemon_client_connected(c, 1000);
    ani_daemon_client_id_t observer = ani_daemon_client_connected(c, 1200);
    ani_daemon_subscription_id_t job_subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ani_daemon_subscription_id_t watch_subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ASSERT_EQ(ani_daemon_job_subscribe(c, owner, "project-lease-job", &job_subscription),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_EQ(ani_daemon_watch_subscribe(c, owner, "project-lease-watch", &watch_subscription),
              ANI_DAEMON_SUBSCRIPTION_STARTED);

    ASSERT_TRUE(ani_daemon_client_heartbeat(c, owner, 1200));
    ASSERT_EQ(ani_daemon_expire_leases(c, 1250), 0);
    ASSERT_EQ(ani_daemon_active_clients(c), 2);
    ASSERT_TRUE(ani_daemon_client_heartbeat(c, observer, 1449));
    ASSERT_EQ(ani_daemon_expire_leases(c, 1449), 0);

    ASSERT_EQ(ani_daemon_expire_leases(c, 1450), 1);
    ASSERT_EQ(ani_daemon_active_clients(c), 1);
    ASSERT_EQ(ani_daemon_coordinator_state(c), ANI_DAEMON_COORDINATOR_RUNNING);
    ASSERT_FALSE(ani_daemon_client_heartbeat(c, owner, 1451));
    ASSERT_FALSE(ani_daemon_client_disconnected(c, owner, 1451));
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-lease-job"), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-lease-job"), ANI_DAEMON_JOB_CANCEL_REQUESTED);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_active_watches(c), 0);
    ASSERT_EQ(tracker.job_cancel_count, 1);
    ASSERT_EQ(tracker.lease_job_cancel_count, 1);
    ASSERT_EQ(tracker.watch_release_count, 1);
    ASSERT_EQ(tracker.lease_watch_release_count, 1);
    ASSERT_EQ(tracker.unexpected_key_count, 0);
    ASSERT_TRUE(ani_daemon_job_reaped(c, "project-lease-job", 1452));
    ASSERT_EQ(ani_daemon_active_jobs(c), 0);

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_last_client_stops_immediately_and_releases_owned_work) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    daemon_release_tracker_t tracker = {0};
    ASSERT_NOT_NULL(c);
    ASSERT_TRUE(install_release_tracker(c, &tracker));

    ani_daemon_client_id_t client = ani_daemon_client_connected(c, 5000);
    ani_daemon_subscription_id_t job_subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ani_daemon_subscription_id_t watch_subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ASSERT_NEQ(client, ANI_DAEMON_CLIENT_ID_INVALID);
    ASSERT_EQ(ani_daemon_coordinator_state(c), ANI_DAEMON_COORDINATOR_RUNNING);
    ASSERT_EQ(ani_daemon_job_subscribe(c, client, "project-shutdown-job", &job_subscription),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_EQ(ani_daemon_watch_subscribe(c, client, "project-shutdown-watch", &watch_subscription),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_NEQ(job_subscription, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_NEQ(watch_subscription, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_active_watches(c), 1);
    ASSERT_EQ(ani_daemon_watch_subscribers(c, "project-shutdown-watch"), 1);

    ASSERT_TRUE(ani_daemon_client_disconnected(c, client, 5200));
    ASSERT_EQ(ani_daemon_active_clients(c), 0);
    ASSERT_EQ(ani_daemon_coordinator_state(c), ANI_DAEMON_COORDINATOR_STOPPING);
    ASSERT_EQ(ani_daemon_active_jobs(c), 1);
    ASSERT_EQ(ani_daemon_active_watches(c), 0);
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-shutdown-job"), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-shutdown-job"), ANI_DAEMON_JOB_CANCEL_REQUESTED);
    ASSERT_EQ(ani_daemon_watch_subscribers(c, "project-shutdown-watch"), 0);
    ASSERT_EQ(tracker.job_cancel_count, 1);
    ASSERT_EQ(tracker.shutdown_job_cancel_count, 1);
    ASSERT_EQ(tracker.watch_release_count, 1);
    ASSERT_EQ(tracker.shutdown_watch_release_count, 1);
    ASSERT_EQ(tracker.unexpected_key_count, 0);

    /* There is no post-client grace before STOPPING, but process exit waits
     * until the cancelled worker is confirmed reaped. */
    ASSERT_FALSE(ani_daemon_should_exit(c, 5200));
    ASSERT_FALSE(ani_daemon_should_exit(c, 10000));

    /* STOPPING is terminal for this daemon instance.  A late connection or a
     * stale connection ID cannot resurrect it or enqueue fresh work. */
    ASSERT_EQ(ani_daemon_client_connected(c, 5201), ANI_DAEMON_CLIENT_ID_INVALID);
    ani_daemon_subscription_id_t rejected = UINT64_MAX;
    ASSERT_EQ(ani_daemon_job_subscribe(c, client, "project-too-late", &rejected),
              ANI_DAEMON_SUBSCRIPTION_REJECTED);
    ASSERT_EQ(rejected, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    rejected = UINT64_MAX;
    ASSERT_EQ(ani_daemon_watch_subscribe(c, client, "project-too-late", &rejected),
              ANI_DAEMON_SUBSCRIPTION_REJECTED);
    ASSERT_EQ(rejected, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_FALSE(ani_daemon_client_disconnected(c, client, 5201));
    ASSERT_EQ(tracker.job_cancel_count, 1);
    ASSERT_EQ(tracker.watch_release_count, 1);

    ASSERT_TRUE(ani_daemon_job_reaped(c, "project-shutdown-job", 5202));
    ASSERT_EQ(ani_daemon_active_jobs(c), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-shutdown-job"), ANI_DAEMON_JOB_NONE);
    ASSERT_TRUE(ani_daemon_should_exit(c, 5202));

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_completed_job_is_not_cancelled_on_later_disconnect) {
    ani_daemon_coordinator_t *c = ani_daemon_coordinator_new(250);
    daemon_release_tracker_t tracker = {0};
    ASSERT_NOT_NULL(c);
    ASSERT_TRUE(install_release_tracker(c, &tracker));

    ani_daemon_client_id_t client = ani_daemon_client_connected(c, 6000);
    ani_daemon_subscription_id_t subscription = ANI_DAEMON_SUBSCRIPTION_ID_INVALID;
    ASSERT_EQ(ani_daemon_job_subscribe(c, client, "project-normal", &subscription),
              ANI_DAEMON_SUBSCRIPTION_STARTED);
    ASSERT_NEQ(subscription, ANI_DAEMON_SUBSCRIPTION_ID_INVALID);
    ASSERT_EQ(ani_daemon_job_state(c, "project-normal"), ANI_DAEMON_JOB_RUNNING);

    ASSERT_TRUE(ani_daemon_job_completed(c, "project-normal", 6100));
    ASSERT_FALSE(ani_daemon_job_completed(c, "project-normal", 6101));
    ASSERT_FALSE(ani_daemon_job_reaped(c, "project-normal", 6101));
    ASSERT_EQ(ani_daemon_active_jobs(c), 0);
    ASSERT_EQ(ani_daemon_job_subscribers(c, "project-normal"), 0);
    ASSERT_EQ(ani_daemon_job_state(c, "project-normal"), ANI_DAEMON_JOB_NONE);

    ASSERT_TRUE(ani_daemon_client_disconnected(c, client, 6200));
    ASSERT_EQ(ani_daemon_coordinator_state(c), ANI_DAEMON_COORDINATOR_STOPPING);
    ASSERT_EQ(tracker.job_cancel_count, 0);
    ASSERT_EQ(tracker.unexpected_key_count, 0);
    ASSERT_TRUE(ani_daemon_should_exit(c, 6200));

    ani_daemon_coordinator_free(c);
    PASS();
}

TEST(daemon_frame_header_rejects_wrong_protocol_and_oversize) {
    uint8_t header[ANI_DAEMON_FRAME_HEADER_SIZE];
    ani_daemon_frame_t frame;

    ASSERT_TRUE(ani_daemon_frame_header_encode(header, ANI_DAEMON_FRAME_REQUEST, 0, 1234));
    ASSERT_TRUE(ani_daemon_frame_header_decode(header, &frame));
    ASSERT_EQ(frame.type, ANI_DAEMON_FRAME_REQUEST);
    ASSERT_EQ(frame.flags, 0);
    ASSERT_EQ(frame.length, 1234);

    header[4] = (uint8_t)(ANI_DAEMON_RENDEZVOUS_FRAME_VERSION + 1);
    ASSERT_FALSE(ani_daemon_frame_header_decode(header, &frame));

    ASSERT_FALSE(ani_daemon_frame_header_encode(header, ANI_DAEMON_FRAME_REQUEST, 0,
                                                ANI_DAEMON_MAX_FRAME_SIZE + 1));
    PASS();
}

static FILE *message_stream_bytes(const void *bytes, size_t length) {
    FILE *f = tmpfile();
    if (!f) {
        return NULL;
    }
    (void)fwrite(bytes, 1, length, f);
    rewind(f);
    return f;
}

static FILE *message_stream(const char *bytes) {
    return message_stream_bytes(bytes, strlen(bytes));
}

TEST(daemon_bridge_reads_newline_and_content_length_messages) {
    char *message = NULL;
    bool framed = false;

    FILE *line = message_stream("{\"jsonrpc\":\"2.0\",\"method\":\"ping\"}\n");
    ASSERT_NOT_NULL(line);
    ASSERT_EQ(ani_mcp_read_message(line, &message, &framed), 1);
    ASSERT_FALSE(framed);
    ASSERT_STR_EQ(message, "{\"jsonrpc\":\"2.0\",\"method\":\"ping\"}");
    free(message);
    message = NULL;
    fclose(line);

    const char *body = "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/list\"}";
    char wire[512];
    snprintf(wire, sizeof(wire), "Content-Length: %zu\r\nX-Test: yes\r\n\r\n%s", strlen(body),
             body);
    FILE *content_length = message_stream(wire);
    ASSERT_NOT_NULL(content_length);
    ASSERT_EQ(ani_mcp_read_message(content_length, &message, &framed), 1);
    ASSERT_TRUE(framed);
    ASSERT_STR_EQ(message, body);
    free(message);
    fclose(content_length);
    PASS();
}

TEST(daemon_bridge_rejects_malformed_content_length) {
    char *message = NULL;
    bool framed = false;
    FILE *wire = message_stream("Content-Length: 4junk\r\n\r\nping");
    ASSERT_NOT_NULL(wire);
    ASSERT_EQ(ani_mcp_read_message(wire, &message, &framed), -1);
    ASSERT_NULL(message);
    fclose(wire);
    PASS();
}

TEST(daemon_bridge_rejects_embedded_nul_body) {
    static const unsigned char wire[] = {
        'C', 'o', 'n', 't', 'e',  'n',  't',  '-',  'L', 'e',  'n', 'g', 't',
        'h', ':', ' ', '4', '\r', '\n', '\r', '\n', 'a', '\0', 'b', 'c',
    };
    char *message = NULL;
    bool framed = false;
    FILE *stream = message_stream_bytes(wire, sizeof(wire));
    ASSERT_NOT_NULL(stream);
    ASSERT_EQ(ani_mcp_read_message(stream, &message, &framed), -1);
    ASSERT_NULL(message);
    fclose(stream);
    PASS();
}

TEST(daemon_bridge_rejects_oversized_headers) {
    const size_t fill_length = 9000;
    const char prefix[] = "Content-Length: 2\r\nX-Fill: ";
    const char suffix[] = "\r\n\r\n{}";
    size_t wire_length = sizeof(prefix) - 1 + fill_length + sizeof(suffix) - 1;
    char *wire_bytes = malloc(wire_length);
    ASSERT_NOT_NULL(wire_bytes);
    memcpy(wire_bytes, prefix, sizeof(prefix) - 1);
    memset(wire_bytes + sizeof(prefix) - 1, 'x', fill_length);
    memcpy(wire_bytes + sizeof(prefix) - 1 + fill_length, suffix, sizeof(suffix) - 1);

    char *message = NULL;
    bool framed = false;
    FILE *stream = message_stream_bytes(wire_bytes, wire_length);
    free(wire_bytes);
    ASSERT_NOT_NULL(stream);
    ASSERT_EQ(ani_mcp_read_message(stream, &message, &framed), -1);
    ASSERT_NULL(message);
    fclose(stream);
    PASS();
}

TEST(daemon_sessions_keep_distinct_roots_and_allowed_root_policy) {
    ani_mcp_server_t *a = ani_mcp_server_new(NULL);
    ani_mcp_server_t *b = ani_mcp_server_new(NULL);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);

    ASSERT_TRUE(ani_mcp_server_set_session_context(a, "/tmp/ani-session-a", "/tmp"));
    ASSERT_TRUE(ani_mcp_server_set_session_context(b, "/tmp/ani-session-b", NULL));
    ani_mcp_server_set_background_tasks(a, false);
    ani_mcp_server_set_background_tasks(b, false);

    ASSERT_STR_EQ(ani_mcp_server_session_root(a), "/tmp/ani-session-a");
    ASSERT_STR_EQ(ani_mcp_server_allowed_root(a), "/tmp");
    ASSERT_STR_EQ(ani_mcp_server_session_root(b), "/tmp/ani-session-b");
    ASSERT_NULL(ani_mcp_server_allowed_root(b));
    ASSERT_STR_NEQ(ani_mcp_server_session_project(a), ani_mcp_server_session_project(b));

    ani_mcp_server_free(a);
    ani_mcp_server_free(b);
    PASS();
}

SUITE(daemon) {
    RUN_TEST(daemon_client_ids_are_connection_bound);
    RUN_TEST(daemon_shared_job_survives_until_final_subscriber_disconnects);
    RUN_TEST(daemon_watch_subscription_ids_are_connection_bound);
    RUN_TEST(daemon_heartbeat_extends_lease_and_expiry_releases_connection);
    RUN_TEST(daemon_last_client_stops_immediately_and_releases_owned_work);
    RUN_TEST(daemon_completed_job_is_not_cancelled_on_later_disconnect);
    RUN_TEST(daemon_frame_header_rejects_wrong_protocol_and_oversize);
    RUN_TEST(daemon_bridge_reads_newline_and_content_length_messages);
    RUN_TEST(daemon_bridge_rejects_malformed_content_length);
    RUN_TEST(daemon_bridge_rejects_embedded_nul_body);
    RUN_TEST(daemon_bridge_rejects_oversized_headers);
    RUN_TEST(daemon_sessions_keep_distinct_roots_and_allowed_root_policy);
}
