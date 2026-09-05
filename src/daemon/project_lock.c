/* project_lock.c — Shared daemon/local-CLI project mutation leases. */
#include "daemon/project_lock.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PROJECT_LOCK_KEY_CAP = 4096 };

static const char PROJECT_SET_KEY[] = "ani-project-set-v1";

struct ani_project_lock_manager {
    ani_private_lock_directory_t *directory;
    ani_lock_registry_t *registry;
};

struct ani_project_lock_lease {
    ani_lock_lease_t *project;
    ani_lock_lease_t *project_set;
};

static bool project_lock_key(const char *project, char out[PROJECT_LOCK_KEY_CAP]) {
    if (!project || !project[0] || strcmp(project, "*") == 0) {
        return false;
    }
    static const char prefix[] = "ani-project-v1:";
    size_t prefix_length = sizeof(prefix) - 1U;
    size_t project_length = strnlen(project, PROJECT_LOCK_KEY_CAP);
    if (project_length == 0 || project_length >= PROJECT_LOCK_KEY_CAP ||
        prefix_length + project_length >= PROJECT_LOCK_KEY_CAP) {
        return false;
    }
    memcpy(out, prefix, prefix_length);
    for (size_t index = 0; index < project_length; index++) {
        unsigned char ch = (unsigned char)project[index];
        out[prefix_length + index] = (char)(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    }
    out[prefix_length + project_length] = '\0';
    return true;
}

ani_project_lock_manager_t *ani_project_lock_manager_new(
    const ani_daemon_ipc_endpoint_t *endpoint) {
    ani_private_lock_directory_t *directory = NULL;
    ani_private_file_lock_status_t directory_status =
        ani_daemon_ipc_private_lock_directory_new(endpoint, &directory);
    if (directory_status != ANI_PRIVATE_FILE_LOCK_OK || !directory) {
        if (directory) {
            ani_private_lock_directory_close(directory);
        }
        return NULL;
    }
    ani_project_lock_manager_t *manager = calloc(1, sizeof(*manager));
    if (manager) {
        manager->directory = directory;
        manager->registry = ani_lock_registry_new(directory);
    }
    if (!manager || !manager->registry) {
        free(manager);
        ani_private_lock_directory_close(directory);
        return NULL;
    }
    return manager;
}

ani_private_file_lock_status_t ani_project_lock_lease_release(ani_project_lock_lease_t **lease_io) {
    if (!lease_io || !*lease_io) {
        return ANI_PRIVATE_FILE_LOCK_IO;
    }
    ani_project_lock_lease_t *lease = *lease_io;
    ani_private_file_lock_status_t result = ANI_PRIVATE_FILE_LOCK_OK;
    if (lease->project) {
        result = ani_lock_lease_release(&lease->project);
        if (lease->project) {
            return ANI_PRIVATE_FILE_LOCK_IO;
        }
    }
    if (lease->project_set) {
        ani_private_file_lock_status_t set_status = ani_lock_lease_release(&lease->project_set);
        if (set_status != ANI_PRIVATE_FILE_LOCK_OK) {
            result = set_status;
        }
        if (lease->project_set) {
            return ANI_PRIVATE_FILE_LOCK_IO;
        }
    }
    free(lease);
    *lease_io = NULL;
    return result;
}

static ani_private_file_lock_status_t project_lock_failed_acquire(
    ani_project_lock_lease_t *lease, ani_private_file_lock_status_t status,
    ani_project_lock_lease_t **lease_out) {
    if (!lease->project && !lease->project_set) {
        free(lease);
        return status;
    }
    ani_project_lock_lease_t *cleanup = lease;
    ani_private_file_lock_status_t cleanup_status = ani_project_lock_lease_release(&cleanup);
    if (cleanup) {
        *lease_out = cleanup;
        return ANI_PRIVATE_FILE_LOCK_IO;
    }
    return cleanup_status == ANI_PRIVATE_FILE_LOCK_OK ? status : ANI_PRIVATE_FILE_LOCK_IO;
}

static ani_private_file_lock_status_t project_lock_acquire_internal(
    ani_project_lock_manager_t *manager, const char *project, uint64_t deadline_ms,
    const ani_lock_cancel_token_t *cancel_token, bool try_once,
    ani_project_lock_lease_t **lease_out) {
    if (lease_out) {
        *lease_out = NULL;
    }
    if (!manager || !manager->registry || !project || !project[0] || !lease_out) {
        return ANI_PRIVATE_FILE_LOCK_UNSAFE;
    }
    bool wildcard = strcmp(project, "*") == 0;
    char project_key[PROJECT_LOCK_KEY_CAP];
    if (!wildcard && !project_lock_key(project, project_key)) {
        return ANI_PRIVATE_FILE_LOCK_UNSAFE;
    }
    ani_project_lock_lease_t *lease = calloc(1, sizeof(*lease));
    if (!lease) {
        return ANI_PRIVATE_FILE_LOCK_IO;
    }
    ani_private_file_lock_status_t status =
        try_once ? ani_lock_registry_try_acquire(manager->registry, PROJECT_SET_KEY,
                                                 wildcard ? ANI_PRIVATE_FILE_LOCK_EX
                                                          : ANI_PRIVATE_FILE_LOCK_SH,
                                                 &lease->project_set)
                 : ani_lock_registry_acquire(manager->registry, PROJECT_SET_KEY,
                                             wildcard ? ANI_PRIVATE_FILE_LOCK_EX
                                                      : ANI_PRIVATE_FILE_LOCK_SH,
                                             deadline_ms, cancel_token, &lease->project_set);
    if (status != ANI_PRIVATE_FILE_LOCK_OK) {
        return project_lock_failed_acquire(lease, status, lease_out);
    }
    if (!wildcard) {
        status = try_once ? ani_lock_registry_try_acquire(manager->registry, project_key,
                                                          ANI_PRIVATE_FILE_LOCK_EX, &lease->project)
                          : ani_lock_registry_acquire(manager->registry, project_key,
                                                      ANI_PRIVATE_FILE_LOCK_EX, deadline_ms,
                                                      cancel_token, &lease->project);
        if (status != ANI_PRIVATE_FILE_LOCK_OK) {
            return project_lock_failed_acquire(lease, status, lease_out);
        }
    }
    *lease_out = lease;
    return ANI_PRIVATE_FILE_LOCK_OK;
}

ani_private_file_lock_status_t ani_project_lock_acquire(ani_project_lock_manager_t *manager,
                                                        const char *project, uint64_t deadline_ms,
                                                        const ani_lock_cancel_token_t *cancel_token,
                                                        ani_project_lock_lease_t **lease_out) {
    return project_lock_acquire_internal(manager, project, deadline_ms, cancel_token, false,
                                         lease_out);
}

ani_private_file_lock_status_t ani_project_lock_try_acquire(ani_project_lock_manager_t *manager,
                                                            const char *project,
                                                            ani_project_lock_lease_t **lease_out) {
    return project_lock_acquire_internal(manager, project, UINT64_MAX, NULL, true, lease_out);
}

ani_private_file_lock_status_t ani_project_lock_request_cancel(ani_project_lock_manager_t *manager,
                                                               ani_lock_cancel_token_t *token) {
    return manager ? ani_lock_registry_request_cancel(manager->registry, token)
                   : ANI_PRIVATE_FILE_LOCK_IO;
}

ani_private_file_lock_status_t ani_project_lock_manager_free(
    ani_project_lock_manager_t **manager_io) {
    if (!manager_io || !*manager_io) {
        return ANI_PRIVATE_FILE_LOCK_IO;
    }
    ani_project_lock_manager_t *manager = *manager_io;
    ani_private_file_lock_status_t status = ani_lock_registry_free(&manager->registry);
    if (status != ANI_PRIVATE_FILE_LOCK_OK) {
        return status;
    }
    ani_private_lock_directory_close(manager->directory);
    manager->directory = NULL;
    free(manager);
    *manager_io = NULL;
    return ANI_PRIVATE_FILE_LOCK_OK;
}
