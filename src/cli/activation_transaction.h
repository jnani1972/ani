/*
 * activation_transaction.h -- Transactional binary activation primitives.
 *
 * This is an internal CLI module.  It deliberately knows nothing about daemon
 * coordination or editor configuration: callers must acquire the maintenance
 * barrier before commit and retain it until finalize/rollback completes.
 */
#ifndef ANI_ACTIVATION_TRANSACTION_H
#define ANI_ACTIVATION_TRANSACTION_H

#include <stdbool.h>
#include <stddef.h>

typedef struct ani_activation_transaction ani_activation_transaction_t;

typedef enum {
    ANI_ACTIVATION_TRANSACTION_OK = 0,
    /* Windows could not unlink an inactive backup (normally because the old
     * executable image is still mapped) but safely registered it for deletion
     * at reboot.  The committed activation remains valid. */
    ANI_ACTIVATION_TRANSACTION_DEFERRED = 1,
    ANI_ACTIVATION_TRANSACTION_INVALID_ARGUMENT = -1,
    ANI_ACTIVATION_TRANSACTION_NO_MEMORY = -2,
    ANI_ACTIVATION_TRANSACTION_IO = -3,
    ANI_ACTIVATION_TRANSACTION_INVALID_STATE = -4,
    /* The post-commit validator rejected the candidate and rollback succeeded. */
    ANI_ACTIVATION_TRANSACTION_VALIDATION_FAILED = -5,
    /* The target changed, but restoring the retained backup also failed. */
    ANI_ACTIVATION_TRANSACTION_ROLLBACK_FAILED = -6,
} ani_activation_transaction_status_t;

typedef bool (*ani_activation_transaction_validator_fn)(const char *target_path, void *context);

/* Test-only seam: invoked after an absent target has been revalidated and
 * immediately before its staged candidate is published.  Production callers
 * leave this unset. */
typedef void (*ani_activation_transaction_before_absent_publish_for_test_fn)(
    const char *target_path, void *context);
void ani_activation_transaction_set_before_absent_publish_for_test(
    ani_activation_transaction_before_absent_publish_for_test_fn hook, void *context);

/* Test-only seam: make the next `count` Windows rename attempts fail as though
 * another handle held the file, so the transient-lock retry can be proven
 * without racing a real scanner. Inert on POSIX and when count is 0. */
void ani_activation_transaction_rename_failures_set_for_test(unsigned int count);

/* Stage a candidate beside target_path (therefore on the same filesystem).
 * The staged file is private to the current account and executable. */
ani_activation_transaction_status_t ani_activation_transaction_stage_bytes(
    const char *target_path, const void *candidate, size_t candidate_size,
    ani_activation_transaction_t **transaction_out);

/* Copy candidate_path into a private executable stage beside target_path. */
ani_activation_transaction_status_t ani_activation_transaction_stage_file(
    const char *target_path, const char *candidate_path,
    ani_activation_transaction_t **transaction_out);

/* Prepare an atomic removal.  A missing target is a valid no-op transaction. */
ani_activation_transaction_status_t ani_activation_transaction_stage_removal(
    const char *target_path, ani_activation_transaction_t **transaction_out);

/* Atomically publish the candidate (or remove the target), retaining any old
 * target at backup_path.  If validator rejects the post-commit state, this
 * function rolls back before returning VALIDATION_FAILED. */
ani_activation_transaction_status_t ani_activation_transaction_commit(
    ani_activation_transaction_t *transaction, ani_activation_transaction_validator_fn validator,
    void *validator_context);

/* Restore the retained target after a successful commit. */
ani_activation_transaction_status_t ani_activation_transaction_rollback(
    ani_activation_transaction_t *transaction);

/* Accept the committed state and delete the retained backup.  On Windows,
 * DEFERRED means deletion was safely registered for reboot; deferred_path
 * remains available for logging until close(). */
ani_activation_transaction_status_t ani_activation_transaction_finalize(
    ani_activation_transaction_t *transaction);

/* Close an object.  An uncommitted object is cleanly aborted; a committed but
 * unfinalized object is rolled back.  On cleanup failure, ownership stays with
 * the caller so paths and rollback can be retried. */
ani_activation_transaction_status_t ani_activation_transaction_close(
    ani_activation_transaction_t **transaction_io);

const char *ani_activation_transaction_target_path(const ani_activation_transaction_t *transaction);
const char *ani_activation_transaction_staged_path(const ani_activation_transaction_t *transaction);
const char *ani_activation_transaction_backup_path(const ani_activation_transaction_t *transaction);
const char *ani_activation_transaction_deferred_path(
    const ani_activation_transaction_t *transaction);

const char *ani_activation_transaction_status_message(ani_activation_transaction_status_t status);

/* Which security predicate refused the most recent transaction, as
 * "predicate (os N)", or "" when nothing refused since the last prepare.
 * The predicates refuse without a usable OS last-error, so this is the only
 * way a caller can say WHY staging failed. Reset by every prepare/stage
 * entry; single-threaded like the rest of the transaction API. */
const char *ani_activation_transaction_refusal_note(void);

#ifdef ANI_ENABLE_TEST_SEAMS
void ani_activation_transaction_note_refusal_for_testing(const char *predicate,
                                                         unsigned long os_error);
#endif

#endif /* ANI_ACTIVATION_TRANSACTION_H */
