#include "scope.h"
#include <string.h>

ANIScope* ani_scope_push(ANIArena* a, ANIScope* current) {
    ANIScope* scope = (ANIScope*)ani_arena_alloc(a, sizeof(ANIScope));
    if (!scope) {
        return current;
    }
    memset(scope, 0, sizeof(ANIScope));
    scope->parent = current;
    scope->arena = a;
    return scope;
}

ANIScope* ani_scope_pop(ANIScope* scope) {
    if (!scope) {
        return NULL;
    }
    return scope->parent;
}

static ANIScopeChunk* alloc_chunk(ANIScope* scope) {
    if (!scope->arena) {
        return NULL;
    }
    ANIScopeChunk* c = (ANIScopeChunk*)ani_arena_alloc(scope->arena, sizeof(ANIScopeChunk));
    if (!c) {
        return NULL;
    }
    memset(c, 0, sizeof(ANIScopeChunk));
    c->next = scope->chunks;
    scope->chunks = c;
    return c;
}

/* Returns false when the binding could NOT be recorded in THIS frame.
 *
 * The failure that matters is arena exhaustion in alloc_chunk: the old void
 * form returned silently, so a caller that then consulted the scope CHAIN saw
 * the parent's binding for the same name and concluded the child had been
 * bound. For callable-value proof that is a fabricated identity -- the shadow
 * never took effect, yet the parent's callable looks like the child's. Callers
 * needing that distinction must use the checked form and consult the LOCAL
 * result, not a chain lookup. */
static bool ani_scope_bind_value(ANIScope *scope, const char *name, const ANIType *type,
                                 const char *callable_qn) {
    if (!scope || !name) {
        return false;
    }
    for (ANIScopeChunk* c = scope->chunks; c != NULL; c = c->next) {
        for (int i = 0; i < c->used; i++) {
            if (c->bindings[i].name && strcmp(c->bindings[i].name, name) == 0) {
                c->bindings[i].type = type;
                c->bindings[i].callable_qn = callable_qn;
                return true;
            }
        }
    }
    ANIScopeChunk* head = scope->chunks;
    if (!head || head->used >= ANI_SCOPE_CHUNK_BINDINGS) {
        head = alloc_chunk(scope);
        if (!head) {
            return false; /* arena exhausted: the shadow did NOT take effect */
        }
    }
    head->bindings[head->used].name = name;
    head->bindings[head->used].type = type;
    head->bindings[head->used].callable_qn = callable_qn;
    head->used++;
    return true;
}

void ani_scope_bind(ANIScope *scope, const char *name, const ANIType *type) {
    (void)ani_scope_bind_value(scope, name, type, NULL);
}

bool ani_scope_bind_checked(ANIScope *scope, const char *name, const ANIType *type) {
    return ani_scope_bind_value(scope, name, type, NULL);
}

void ani_scope_bind_callable(ANIScope *scope, const char *name, const ANIType *type,
                             const char *callable_qn) {
    (void)ani_scope_bind_value(scope, name, type, callable_qn);
}

bool ani_scope_bind_callable_checked(ANIScope *scope, const char *name, const ANIType *type,
                                     const char *callable_qn) {
    return ani_scope_bind_value(scope, name, type, callable_qn);
}

const ANIType* ani_scope_lookup(const ANIScope* scope, const char* name) {
    if (!name) {
        return ani_type_unknown();
    }
    for (const ANIScope* s = scope; s != NULL; s = s->parent) {
        for (ANIScopeChunk* c = s->chunks; c != NULL; c = c->next) {
            for (int i = 0; i < c->used; i++) {
                if (c->bindings[i].name && strcmp(c->bindings[i].name, name) == 0) {
                    return c->bindings[i].type;
                }
            }
        }
    }
    return ani_type_unknown();
}

bool ani_scope_contains(const ANIScope *scope, const char *name) {
    if (!name) {
        return false;
    }
    for (const ANIScope *s = scope; s != NULL; s = s->parent) {
        for (const ANIScopeChunk *c = s->chunks; c != NULL; c = c->next) {
            for (int i = 0; i < c->used; i++) {
                if (c->bindings[i].name && strcmp(c->bindings[i].name, name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

const char *ani_scope_lookup_callable(const ANIScope *scope, const char *name) {
    if (!name) {
        return NULL;
    }
    for (const ANIScope *s = scope; s != NULL; s = s->parent) {
        for (const ANIScopeChunk *c = s->chunks; c != NULL; c = c->next) {
            for (int i = 0; i < c->used; i++) {
                if (c->bindings[i].name && strcmp(c->bindings[i].name, name) == 0) {
                    return c->bindings[i].callable_qn;
                }
            }
        }
    }
    return NULL;
}

bool ani_scope_update_callable(ANIScope *scope, const char *name, const char *callable_qn) {
    if (!name) {
        return false;
    }
    for (ANIScope *s = scope; s != NULL; s = s->parent) {
        for (ANIScopeChunk *c = s->chunks; c != NULL; c = c->next) {
            for (int i = 0; i < c->used; i++) {
                if (c->bindings[i].name && strcmp(c->bindings[i].name, name) == 0) {
                    c->bindings[i].callable_qn = callable_qn;
                    return true;
                }
            }
        }
    }
    return false;
}
