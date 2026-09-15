/**
 * @file cli-api-console.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief linenoise/esp_console wiring and prompt configuration.
 *
 * Writes `s_cli.prompt`. Command history itself is always kept in RAM only by
 * linenoise's own history buffer (see cli-api.c); this file never touches
 * flash — see cli-api-storage.c for the opt-in 'sync' command that does.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "cli-api-private.h"

#include <esp_console.h>
#include <linenoise/linenoise.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>

void cli_init_linenoise(void)
{
  /* Initialize esp_console */
  esp_console_config_t console_config = {.max_cmdline_args = CLI_MAX_CMDLINE_ARGS,
                                         .max_cmdline_length = CLI_MAX_CMDLINE_LENGTH,
#if CONFIG_LOG_COLORS
                                         .hint_color = atoi(LOG_COLOR_CYAN)
#endif
  };
  ESP_ERROR_CHECK(esp_console_init(&console_config));

  /* Configure linenoise */
  linenoiseSetDumbMode(1);   /* Required for Web Serial / dumb terminals (no ANSI/VT100) */
  linenoiseSetMultiLine(1);
  linenoiseSetCompletionCallback(&esp_console_get_completion);
  linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);
  linenoiseHistorySetMaxLen(CLI_HISTORY_SIZE);
  linenoiseSetMaxLineLen(CLI_MAX_CMDLINE_LENGTH);
  linenoiseAllowEmpty(false);

  /* Detect escape sequences support */
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  /* USB Serial JTAG: skip detection, assume smart terminal */
  linenoiseSetDumbMode(0);
#else
  const int probe_status = linenoiseProbe();
  if (probe_status)
    linenoiseSetDumbMode(1);
#endif
}

void cli_setup_prompt(const char *prompt_str)
{
  const char *prompt_temp = "esp> ";
  if (prompt_str != NULL)
    prompt_temp = prompt_str;

#if CONFIG_LOG_COLORS
  if (!linenoiseIsDumbMode())
    snprintf(s_cli.prompt, CLI_PROMPT_MAX_LEN - 1, LOG_COLOR_I "%s" LOG_RESET_COLOR, prompt_temp);
  else
    snprintf(s_cli.prompt, CLI_PROMPT_MAX_LEN - 1, "%s", prompt_temp);
#else
  snprintf(s_cli.prompt, CLI_PROMPT_MAX_LEN - 1, "%s", prompt_temp);
#endif
}
