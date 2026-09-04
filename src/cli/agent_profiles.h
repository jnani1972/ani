/*
 * agent_profiles.h — Canonical tiered ani agent profiles.
 */
#ifndef ANI_CLI_AGENT_PROFILES_H
#define ANI_CLI_AGENT_PROFILES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ANI_GRAPH_TIER_SCOUT = 0,
    ANI_GRAPH_TIER_VERIFY,
    ANI_GRAPH_TIER_AUDIT,
    ANI_GRAPH_TIER_COUNT
} ani_graph_tier_t;

typedef enum {
    ANI_GRAPH_ACCESS_DIRECT = 0,
    ANI_GRAPH_ACCESS_HANDOFF,
    ANI_GRAPH_ACCESS_COUNT
} ani_graph_access_t;

typedef enum {
    ANI_GRAPH_DIALECT_CLAUDE = 0,
    ANI_GRAPH_DIALECT_CODEX,
    ANI_GRAPH_DIALECT_GEMINI,
    ANI_GRAPH_DIALECT_QWEN,
    ANI_GRAPH_DIALECT_COPILOT,
    ANI_GRAPH_DIALECT_OPENCODE,
    ANI_GRAPH_DIALECT_KILO,
    ANI_GRAPH_DIALECT_KIRO,
    ANI_GRAPH_DIALECT_JUNIE,
    ANI_GRAPH_DIALECT_QODER,
    ANI_GRAPH_DIALECT_CODEBUDDY,
    ANI_GRAPH_DIALECT_FACTORY,
    ANI_GRAPH_DIALECT_VIBE,
    ANI_GRAPH_DIALECT_AUGMENT,
    ANI_GRAPH_DIALECT_CURSOR,
    ANI_GRAPH_DIALECT_ROVO,
    ANI_GRAPH_DIALECT_POCHI,
    ANI_GRAPH_DIALECT_OMP,
    ANI_GRAPH_DIALECT_GROK,
    ANI_GRAPH_DIALECT_COUNT
} ani_graph_profile_dialect_t;

/* Stable profile identifier. VERIFY intentionally retains "ani". */
const char *ani_graph_tier_slug(ani_graph_tier_t tier);
const char *ani_graph_tier_display_name(ani_graph_tier_t tier);
bool ani_graph_dialect_direct_capable(ani_graph_profile_dialect_t dialect);

/* Returns malloc-owned profile content, or NULL for invalid/unsafe combinations.
 * binary_path is required for direct Kiro and Codex profiles and ignored otherwise. */
char *ani_render_graph_profile(ani_graph_profile_dialect_t dialect, ani_graph_tier_t tier,
                               ani_graph_access_t access, const char *binary_path);

/* v0.9.1-rc.1 direct Codex rendering (server table without a transport), kept
 * so install/uninstall can recognize and migrate those files. */
char *ani_render_graph_profile_codex_rc1(ani_graph_tier_t tier);

/* Vibe stores the behavioral prompt separately from its TOML agent definition.
 * Other integrations may also use this as the canonical contract text. */
char *ani_render_graph_prompt(ani_graph_tier_t tier, ani_graph_access_t access);

#ifdef __cplusplus
}
#endif

#endif /* ANI_CLI_AGENT_PROFILES_H */
