/**
 * @file cli-api-history.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief RAM copy of the command history and the optional 'history' command.
 *
 * linenoise keeps its history array private (it only offers add/save/load/free),
 * so this file keeps a copy in `s_cli.history[]` that mirrors it entry for
 * entry, purely so 'history' can print it. The copy is only kept once 'history'
 * is registered, so consumers that never register it don't pay for it.
 *
 * This is the only file that touches `s_cli.history[]`, `s_cli.history_len`
 * and `s_cli.history_mirror`.
 *
 * @version 0.1
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <linenoise/linenoise.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli-api-private.h"

void cli_history_add(const char *line)
{
  /* linenoise skips consecutive duplicates (returns 0); mirroring only what it
   * actually added keeps both lists identical */
  if (linenoiseHistoryAdd(line) != 1 || !s_cli.history_mirror)
    return;

  char *copy = strdup(line);
  if (copy == NULL)
    return;

  /* Full: drop the oldest entry, same as linenoise does */
  if (s_cli.history_len == CLI_HISTORY_SIZE)
  {
    free(s_cli.history[0]);
    memmove(&s_cli.history[0], &s_cli.history[1], sizeof(s_cli.history[0]) * (CLI_HISTORY_SIZE - 1));
    s_cli.history_len--;
  }

  s_cli.history[s_cli.history_len++] = copy;
}

void cli_history_clear(void)
{
  linenoiseHistoryFree();

  for (int i = 0; i < s_cli.history_len; i++)
  {
    free(s_cli.history[i]);
    s_cli.history[i] = NULL;
  }
  s_cli.history_len = 0;
}

/**
 * @brief "history" command: prints the in-memory history, numbered from 1, or
 * clears it (including UP/DOWN recall) with -c/--clear.
 */
static int cmd_history(cli_context_t *ctx)
{
  if (ctx->args[0].flag_value)
  {
    cli_history_clear();
    printf("History cleared\n");
    return 0;
  }

  for (int i = 0; i < s_cli.history_len; i++) printf("%4d  %s\n", i + 1, s_cli.history[i]);

  return 0;
}

/* Static: cli_register_command() keeps a pointer to it */
static const cli_command_t s_history_cmd = {
  .name = "history",
  .description = "Show the in-memory command history",
  .hint = NULL,
  .callback = cmd_history,
  .args =
    {
      {.short_opt = "c",
       .long_opt = "clear",
       .datatype = NULL,
       .description = "Clear the history (also UP/DOWN recall)",
       .type = CLI_ARG_TYPE_FLAG,
       .required = false},
    },
  .arg_count = 1,
};

void cli_register_history_command(void)
{
  s_cli.history_mirror = true;
  cli_register_command(&s_history_cmd);
}
