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

#include <esp_console.h>
#include <linenoise/linenoise.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cli-api-private.h"

#define ASCII_BS  0x08
#define ASCII_DEL 0x7F

/**
 * @brief linenoise read function that turns DEL into BS.
 *
 * Most terminals send DEL for the Backspace key. In dumb mode linenoise erases
 * with "\b " and then echoes the key it received; DEL is not rendered, so the
 * cursor would stay one column to the right. Echoing BS instead moves it back.
 * Line-editing mode treats both keys as backspace, so it is unaffected.
 */
static ssize_t cli_read_bytes(int fd, void *buf, size_t count)
{
  ssize_t nread = read(fd, buf, count);
  char *bytes = buf;
  for (ssize_t i = 0; i < nread; i++)
    if (bytes[i] == ASCII_DEL)
      bytes[i] = ASCII_BS;

  return nread;
}

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
  linenoiseSetMultiLine(1);
  linenoiseSetCompletionCallback(&esp_console_get_completion);
  linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);
  linenoiseHistorySetMaxLen(CLI_HISTORY_SIZE);
  linenoiseSetMaxLineLen(CLI_MAX_CMDLINE_LENGTH);
  linenoiseAllowEmpty(false);

  /* Select line editing or dumb mode (CLI API Configuration > Terminal mode) */
#if CONFIG_CLI_API_TERMINAL_MODE_DUMB
  /* Web Serial / hosts without ANSI/VT100 support */
  linenoiseSetDumbMode(1);
#elif CONFIG_CLI_API_TERMINAL_MODE_LINE_EDITING || defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  /* Forced, or USB Serial JTAG: the host is usually not attached yet at boot,
   * so the probe would fail — skip detection and assume a smart terminal */
  linenoiseSetDumbMode(0);
#else
  /* Auto: dumb mode only if the terminal does not answer the probe */
  linenoiseSetDumbMode(linenoiseProbe() != 0);
#endif

  /* Must come after linenoiseProbe(), which resets the read function to read() */
  linenoiseSetReadFunction(cli_read_bytes);
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
