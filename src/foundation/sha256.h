#ifndef ANI_SHA256_H
#define ANI_SHA256_H

/* In-process SHA-256 (FIPS 180-4). Used to verify the integrity of a
 * downloaded release before installing it, without shelling out to a
 * platform hashing tool (shasum / sha256sum / certutil) — those differ per
 * OS, may be absent, and mis-quote paths under cmd.exe. */

#include <stddef.h>
#include <stdint.h>

#define ANI_SHA256_DIGEST_LEN 32 /* raw digest bytes */
#define ANI_SHA256_HEX_LEN 64    /* lowercase hex chars (no NUL) */

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t buf[64];
    size_t buflen;
} ani_sha256_ctx;

void ani_sha256_init(ani_sha256_ctx *c);
void ani_sha256_update(ani_sha256_ctx *c, const void *data, size_t len);
void ani_sha256_final(ani_sha256_ctx *c, uint8_t out[ANI_SHA256_DIGEST_LEN]);

/* One-shot hash of a buffer to lowercase hex. `out` must hold
 * ANI_SHA256_HEX_LEN + 1 bytes (hex chars + NUL). */
void ani_sha256_hex(const void *data, size_t len, char out[ANI_SHA256_HEX_LEN + 1]);

/* RFC 2104 HMAC-SHA-256. The output is always ANI_SHA256_DIGEST_LEN bytes.
 * A NULL key/data pointer is accepted only when its corresponding length is
 * zero. */
void ani_hmac_sha256(const void *key, size_t key_len, const void *data, size_t data_len,
                     uint8_t out[ANI_SHA256_DIGEST_LEN]);

#endif /* ANI_SHA256_H */
