/*
 * embedded_stub.c — Empty asset table when built without frontend.
 *
 * Used by the standard `ani` target (no Node.js required).
 * The `ani-with-ui` target replaces this with generated embedded_assets.c.
 */
#include "ui/embedded_assets.h"

#include <stddef.h>
#include <string.h>

ani_embedded_file_t ANI_EMBEDDED_FILES[] = {{NULL, NULL, 0, NULL}};
const int ANI_EMBEDDED_FILE_COUNT = 0;

const ani_embedded_file_t *ani_embedded_lookup(const char *path) {
    for (int i = 0; i < ANI_EMBEDDED_FILE_COUNT; i++) {
        if (strcmp(ANI_EMBEDDED_FILES[i].path, path) == 0) {
            return &ANI_EMBEDDED_FILES[i];
        }
    }
    return NULL;
}
