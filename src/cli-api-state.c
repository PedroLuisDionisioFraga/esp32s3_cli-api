/**
 * @file cli-api-state.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief Owns the single CLI state instance.
 *
 * This is the only file that defines `s_cli`. cli_init/cli_run/cli_deinit and
 * command registration are all invoked synchronously from a single task (no
 * ISR or concurrent task touches this state), so the other src/*.c files
 * access its fields directly through the `extern` declaration in
 * cli-api-private.h rather than through accessor functions.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "cli-api-private.h"

cli_state_t s_cli = {
  .prompt = "esp32-cli> ",
  .initialized = false,
  .wl_handle = WL_INVALID_HANDLE,
  .cmd_count = 0,
};
