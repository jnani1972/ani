/* RED contract for daemon/local-CLI cross-process project coordination. */
#include "test_framework.h"
#include "test_helpers.h"

#include "daemon/ipc.h"
#include "daemon/project_lock.h"
#include "foundation/compat_fs.h"
#include "foundation/platform.h"

#include <stdint.h>
#include <stdio.h>

enum { PROJECT_LOCK_TEST_PATH_CAP = 1024 };

static void project_lock_test_release(ani_project_lock_lease_t **lease) {
    while (lease && *lease && ani_project_lock_lease_release(lease) != ANI_PRIVATE_FILE_LOCK_OK) {
        ani_usleep(1000);
    }
}

TEST(project_lock_coordinates_instances_projects_wildcard_and_case_aliases) {
    char runtime_parent[PROJECT_LOCK_TEST_PATH_CAP];
    (void)snprintf(runtime_parent, sizeof(runtime_parent), "%s/ani-project-lock-XXXXXX",
                   ani_tmpdir());
    ASSERT_NOT_NULL(ani_mkdtemp(runtime_parent));

    ani_daemon_ipc_endpoint_t *endpoint =
        ani_daemon_ipc_endpoint_new("0123456789abcdef", runtime_parent);
    ani_project_lock_manager_t *first = ani_project_lock_manager_new(endpoint);
    ani_project_lock_manager_t *second = ani_project_lock_manager_new(endpoint);
    ASSERT_NOT_NULL(endpoint);
    ASSERT_NOT_NULL(first);
    ASSERT_NOT_NULL(second);

    ani_project_lock_lease_t *foo = NULL;
    ani_project_lock_lease_t *alias = NULL;
    ani_project_lock_lease_t *bar = NULL;
    ASSERT_EQ(ani_project_lock_try_acquire(first, "Foo", &foo), ANI_PRIVATE_FILE_LOCK_OK);
    ASSERT_NOT_NULL(foo);
    ASSERT_EQ(ani_project_lock_try_acquire(second, "foo", &alias), ANI_PRIVATE_FILE_LOCK_BUSY);
    ASSERT_NULL(alias);
    ASSERT_EQ(ani_project_lock_acquire(second, "bar", UINT64_MAX, NULL, &bar),
              ANI_PRIVATE_FILE_LOCK_OK);
    ASSERT_NOT_NULL(bar);

    project_lock_test_release(&bar);
    project_lock_test_release(&foo);

    ani_project_lock_lease_t *all = NULL;
    ASSERT_EQ(ani_project_lock_acquire(first, "*", UINT64_MAX, NULL, &all),
              ANI_PRIVATE_FILE_LOCK_OK);
    ASSERT_NOT_NULL(all);
    ASSERT_EQ(ani_project_lock_try_acquire(second, "unrelated", &bar), ANI_PRIVATE_FILE_LOCK_BUSY);
    ASSERT_NULL(bar);

    project_lock_test_release(&all);
    ASSERT_EQ(ani_project_lock_manager_free(&second), ANI_PRIVATE_FILE_LOCK_OK);
    ASSERT_EQ(ani_project_lock_manager_free(&first), ANI_PRIVATE_FILE_LOCK_OK);
    ASSERT_NULL(second);
    ASSERT_NULL(first);
    ani_daemon_ipc_endpoint_free(endpoint);
    (void)th_rmtree(runtime_parent);
    PASS();
}

SUITE(project_lock) {
    RUN_TEST(project_lock_coordinates_instances_projects_wildcard_and_case_aliases);
}
