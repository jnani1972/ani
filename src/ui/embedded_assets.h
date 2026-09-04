/*
 * embedded_assets.h — Interface for embedded frontend assets.
 *
 * When built with `ani-with-ui`, the embed script generates embedded_assets.c
 * with actual file data. Otherwise, embedded_stub.c provides an empty array.
 */
#ifndef ANI_UI_EMBEDDED_ASSETS_H
#define ANI_UI_EMBEDDED_ASSETS_H

typedef struct {
    const char *path;          /* URL path, e.g. "/index.html" */
    const unsigned char *data; /* raw file bytes */
    unsigned int size;         /* byte count */
    const char *content_type;  /* MIME type, e.g. "text/html" */
} ani_embedded_file_t;

/* Array of embedded files + count. Defined in embedded_assets.c or embedded_stub.c.
 * Not const — sizes are initialized at startup by a constructor function. */
extern ani_embedded_file_t ANI_EMBEDDED_FILES[];
extern const int ANI_EMBEDDED_FILE_COUNT;

/* Look up an embedded file by URL path. Returns NULL if not found. */
const ani_embedded_file_t *ani_embedded_lookup(const char *path);

#endif /* ANI_UI_EMBEDDED_ASSETS_H */
