/*
 * compat_thread.h — Portable threading: pthreads on POSIX, Win32 threads on Windows.
 *
 * Provides: thread create/join, mutex, aligned allocation.
 * All have zero overhead on POSIX (thin inlines or macros).
 */
#ifndef ANI_COMPAT_THREAD_H
#define ANI_COMPAT_THREAD_H

#include <stddef.h>

/* ── Thread ───────────────────────────────────────────────────── */

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct {
    HANDLE handle;
} ani_thread_t;

#else /* POSIX */

#include <pthread.h>

typedef struct {
    pthread_t handle;
} ani_thread_t;

#endif

/* Create a thread with the given stack size (0 = OS default).
 * fn receives arg. Returns 0 on success. */
int ani_thread_create(ani_thread_t *t, size_t stack_size, void *(*fn)(void *), void *arg);

/* Wait for thread to finish. Returns 0 on success. */
int ani_thread_join(ani_thread_t *t);

/* Detach thread so resources are freed on exit. Returns 0 on success. */
int ani_thread_detach(ani_thread_t *t);

/* ── Mutex ────────────────────────────────────────────────────── */

#ifdef _WIN32

typedef struct {
    CRITICAL_SECTION cs;
} ani_mutex_t;

#else

typedef struct {
    pthread_mutex_t mtx;
} ani_mutex_t;

#endif

void ani_mutex_init(ani_mutex_t *m);
void ani_mutex_lock(ani_mutex_t *m);
void ani_mutex_unlock(ani_mutex_t *m);
void ani_mutex_destroy(ani_mutex_t *m);

/* ── Aligned allocation ───────────────────────────────────────── */

/* Allocate size bytes aligned to alignment boundary.
 * Returns 0 on success, non-zero on failure. *ptr receives the allocation. */
int ani_aligned_alloc(void **ptr, size_t alignment, size_t size);

/* Free memory from ani_aligned_alloc. */
void ani_aligned_free(void *ptr);

#endif /* ANI_COMPAT_THREAD_H */
