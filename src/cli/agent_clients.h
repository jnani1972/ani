/*
 * agent_clients.h — Table-driven agent client MCP installation profiles.
 */
#ifndef ANI_CLI_AGENT_CLIENTS_H
#define ANI_CLI_AGENT_CLIENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ANI_AGENT_CLIENT_QODER = 0,
    ANI_AGENT_CLIENT_KIMI,
    ANI_AGENT_CLIENT_GITLAB_DUO,
    ANI_AGENT_CLIENT_ROVO_DEV,
    ANI_AGENT_CLIENT_AMP,
    ANI_AGENT_CLIENT_DEVIN,
    ANI_AGENT_CLIENT_TABNINE,
    ANI_AGENT_CLIENT_CONTINUE,
    ANI_AGENT_CLIENT_VISUAL_STUDIO,
    ANI_AGENT_CLIENT_TRAE,
    ANI_AGENT_CLIENT_ROO_CODE,
    ANI_AGENT_CLIENT_AMAZON_Q,
    ANI_AGENT_CLIENT_CODEBUDDY,
    ANI_AGENT_CLIENT_IBM_BOB_IDE,
    ANI_AGENT_CLIENT_IBM_BOB_SHELL,
    ANI_AGENT_CLIENT_POCHI,
    ANI_AGENT_CLIENT_PI,
    ANI_AGENT_CLIENT_SOURCEGRAPH_CODY,
    ANI_AGENT_CLIENT_OMP,
    ANI_AGENT_CLIENT_COUNT
} ani_agent_client_id_t;

typedef enum {
    ANI_AGENT_STABLE = 0,
    ANI_AGENT_CONDITIONAL,
    ANI_AGENT_OPT_IN
} ani_agent_client_stability_t;

enum {
    ANI_AGENT_CAP_MCP = UINT32_C(1) << 0,
    ANI_AGENT_CAP_INSTRUCTIONS = UINT32_C(1) << 1,
    ANI_AGENT_CAP_SKILL = UINT32_C(1) << 2,
    ANI_AGENT_CAP_AGENT = UINT32_C(1) << 3,
    ANI_AGENT_CAP_HOOK = UINT32_C(1) << 4,
    ANI_AGENT_CAP_PLUGIN = UINT32_C(1) << 5
};

typedef int (*ani_agent_mcp_edit_fn)(ani_agent_client_id_t id, const char *config_path,
                                     const char *binary_path);

typedef struct {
    ani_agent_client_id_t id;
    const char *stable_id;
    const char *display_name;
    ani_agent_client_stability_t stability;
    uint32_t capabilities;
    const char *detection_command;
    ani_agent_mcp_edit_fn install_mcp;
    ani_agent_mcp_edit_fn remove_mcp;
} ani_agent_client_profile_t;

typedef bool (*ani_agent_probe_fn)(const char *value, const void *context);

typedef struct {
    const char *home_dir;
    const char *xdg_config_home;
    const char *appdata_dir;
    const char *glab_config_dir;
    const char *kimi_code_home;
    const char *continue_config_path;
    const char *trae_config_path;
    const char *roo_config_path;
    const char *cody_config_path;
    const char *omp_agent_dir;
    bool is_windows;
    ani_agent_probe_fn path_exists;
    ani_agent_probe_fn command_exists;
    const void *probe_context;
} ani_agent_client_resolve_options_t;

enum {
    ANI_AGENT_EDIT_ERROR = -1,
    ANI_AGENT_EDIT_OK = 0,
    ANI_AGENT_EDIT_FOREIGN = 1,
    ANI_AGENT_EDIT_NOT_APPLICABLE = 2
};

size_t ani_agent_client_count(void);
const ani_agent_client_profile_t *ani_agent_client_at(size_t index);
const ani_agent_client_profile_t *ani_agent_client_by_id(ani_agent_client_id_t id);
const ani_agent_client_profile_t *ani_agent_client_by_stable_id(const char *stable_id);

/* Resolves the documented user config path. Returns 0 on success, 1 when a
 * conditional target has no safe active path, and -1 for invalid input or an
 * ambiguous/unsupported configuration. */
int ani_agent_client_resolve_path(ani_agent_client_id_t id,
                                  const ani_agent_client_resolve_options_t *options, char *path_out,
                                  size_t path_out_size);
bool ani_agent_client_detect(ani_agent_client_id_t id,
                             const ani_agent_client_resolve_options_t *options);
bool ani_agent_client_cleanup_candidate(ani_agent_client_id_t id,
                                        const ani_agent_client_resolve_options_t *options);

/* config_path must already have been resolved. The adapter never guesses a
 * target here. Existing same-name foreign entries fail closed with
 * ANI_AGENT_EDIT_FOREIGN. Removal requires the original installed binary path
 * and only removes the still-canonical entry. */
int ani_agent_client_install_mcp(ani_agent_client_id_t id, const char *config_path,
                                 const char *binary_path);
int ani_agent_client_remove_mcp(ani_agent_client_id_t id, const char *config_path,
                                const char *binary_path);

#ifdef __cplusplus
}
#endif

#endif /* ANI_CLI_AGENT_CLIENTS_H */
