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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cli-api-private.h"

#define ASCII_BS  0x08
#define ASCII_ESC 0x1B
#define ASCII_DEL 0x7F

/* Where we are inside an escape sequence that cli_keep_byte() is dropping */
typedef enum
{
  ESC_NONE,  /* not in a sequence */
  ESC_START, /* got ESC, next byte tells which kind */
  ESC_CSI,   /* got ESC [, params until a final byte 0x40-0x7E */
  ESC_SS3,   /* got ESC O, one final byte left */
} esc_state_t;

static esc_state_t s_esc_state = ESC_NONE;

/**
 * @brief Whether a received byte should reach linenoise.
 *
 * In dumb mode linenoise drops the ESC byte itself but keeps and echoes the
 * rest ("[A" for an arrow key, "[0n" for a probe reply), which then ends up
 * in the command line and in the history. Dropping the whole sequence here
 * keeps that out. Line-editing mode needs the sequences, so they pass through.
 */
static bool cli_keep_byte(unsigned char c)
{
  switch (s_esc_state)
  {
    case ESC_NONE:
      if (c != ASCII_ESC || !linenoiseIsDumbMode())
        return true;
      s_esc_state = ESC_START;
      return false;

    case ESC_START:
      if (c < 0x20) /* ESC followed by a control key (e.g. Enter): not a sequence */
      {
        s_esc_state = ESC_NONE;
        return true;
      }
      s_esc_state = (c == '[') ? ESC_CSI : (c == 'O') ? ESC_SS3 : ESC_NONE;
      return false;

    case ESC_SS3:
      s_esc_state = ESC_NONE;
      return false;

    case ESC_CSI:
      if (c < 0x20) /* truncated sequence: keep the control key (Enter) */
      {
        s_esc_state = ESC_NONE;
        return true;
      }
      if (c >= 0x40 && c <= 0x7E)
        s_esc_state = ESC_NONE;
      return false;
  }

  return true;
}

/**
 * @brief linenoise read function: drops escape sequences in dumb mode and
 * turns DEL into BS.
 *
 * Most terminals send DEL for the Backspace key. In dumb mode linenoise erases
 * with "\b " and then echoes the key it received; DEL is not rendered, so the
 * cursor would stay one column to the right. Echoing BS instead moves it back.
 * Line-editing mode treats both keys as backspace, so it is unaffected.
 */
static ssize_t cli_read_bytes(int fd, void *buf, size_t count)
{
  char *bytes = buf;
  ssize_t nread;
  ssize_t kept;

  /* Read again if everything we got was part of a dropped sequence */
  do
  {
    nread = read(fd, buf, count);
    kept = 0;
    for (ssize_t i = 0; i < nread; i++)
    {
      if (!cli_keep_byte((unsigned char)bytes[i]))
        continue;

      bytes[kept++] = (bytes[i] == ASCII_DEL) ? ASCII_BS : bytes[i];
    }
  } while (nread > 0 && kept == 0);

  return nread < 0 ? nread : kept;
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
