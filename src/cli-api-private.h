/**
 * @file cli-api-private.h
 * @brief Internal types and helpers shared across src/cli-api-*.c
 *
 * Nothing declared here is public API: this header is never added to
 * INCLUDE_DIRS, so it is unreachable from outside the component. The `cli_`
 * prefix on the non-static helpers below exists only to avoid link-time
 * collisions, not to imply public API status.
 *
 * @author Pedro Luis Dionisio Fraga (pedrodfraga@hotmail.com)
 * @date 2026
 */

#ifndef CLI_API_PRIVATE_H
#define CLI_API_PRIVATE_H

#include <esp_vfs_fat.h> /* wl_handle_t */

#include "cli-api.h"

/* Mount path for the FATFS partition used to persist command history */
#define CLI_MOUNT_PATH   "/data"
#define CLI_HISTORY_PATH CLI_MOUNT_PATH "/history.txt"

/**
 * @brief Internally registered command: user definition + argtable3 table
 */
typedef struct
{
  const cli_command_t *cmd_def;     /**< Original command definition */
  void *argtable[CLI_MAX_ARGS + 1]; /**< Pointers to argtable3 (+1 for arg_end) */
  uint8_t arg_count;                /**< Number of arguments */
} cli_registered_cmd_t;

/**
 * @brief Internal CLI singleton state
 */
typedef struct
{
  char prompt[CLI_PROMPT_MAX_LEN];             /**< Console prompt string */
  bool initialized;                            /**< true if console was initialized */
  wl_handle_t wl_handle;                       /**< Wear-levelling handle for FATFS, WL_INVALID_HANDLE
                                                *   until 'sync' mounts it for the first time */
  cli_registered_cmd_t cmds[CLI_MAX_COMMANDS]; /**< Registered commands */
  uint8_t cmd_count;                           /**< Number of registered commands */
  char *history[CLI_HISTORY_SIZE];             /**< Copy of linenoise's history, oldest first, for 'history' */
  uint16_t history_len;                        /**< Number of entries in history[] */
  bool history_mirror;                         /**< true once 'history' is registered; only then is
                                                *   history[] kept up to date */
} cli_state_t;

/**
 * @brief The one CLI state instance, defined in cli-api-state.c
 */
extern cli_state_t s_cli;

/* -------------------------------------------------------------------------- */
/*                  Cross-file helpers (implemented elsewhere)                */
/* -------------------------------------------------------------------------- */

/** @brief Initialize NVS (Non-Volatile Storage). Implemented in cli-api-storage.c */
esp_err_t cli_init_nvs(void);

/** @brief Mount the FATFS filesystem used to store history. Implemented in cli-api-storage.c */
esp_err_t cli_init_filesystem(void);

/** @brief Unmount the FATFS filesystem. Implemented in cli-api-storage.c */
void cli_deinit_filesystem(void);

/** @brief Bring up the console peripheral (UART, USB CDC or USB Serial JTAG). Implemented in cli-api-peripheral.c */
void cli_init_peripheral(void);

/** @brief Initialize linenoise and esp_console. Implemented in cli-api-console.c */
void cli_init_linenoise(void);

/** @brief Configure the console prompt string. Implemented in cli-api-console.c */
void cli_setup_prompt(const char *prompt_str);

/** @brief Register the optional 'sync' command (persists history to flash on demand).
 *  Implemented in cli-api-storage.c */
void cli_register_history_sync_command(void);

/** @brief Add a line to linenoise's history (and to the copy 'history' prints). Implemented in cli-api-history.c */
void cli_history_add(const char *line);

/** @brief Add every line of a saved history file via cli_history_add(); silent if the file doesn't exist.
 *  Implemented in cli-api-history.c */
void cli_history_load(const char *path);

/** @brief Clear linenoise's history and the copy 'history' prints. Implemented in cli-api-history.c */
void cli_history_clear(void);

/** @brief Register the optional 'history' command (list, or clear with -c). Implemented in cli-api-history.c */
void cli_register_history_command(void);

#endif /* CLI_API_PRIVATE_H */
