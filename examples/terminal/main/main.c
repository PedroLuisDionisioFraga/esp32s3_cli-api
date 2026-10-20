/*
 * SPDX-FileCopyrightText: 2026 Pedro Luis Dionisio Fraga
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/**
 * @file terminal_example_main.c
 * @brief Terminal features example: line editing, terminal mode and command history
 *
 * This example demonstrates:
 * - The "CLI API Configuration > Terminal mode" option (Auto / Force line editing / Force dumb)
 * - Line editing: TAB completion, argument hints, UP/DOWN history and Backspace
 * - The 'history' (and 'history -c') and 'sync' commands registered by history_sync = true
 */

#include <linenoise/linenoise.h>
#include <stdio.h>

#include "cli-api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "terminal_example";

/* ========================================================================== */
/*                         TERM (terminal mode report)                        */
/* ========================================================================== */

/**
 * @brief "term" command - prints the configured terminal mode and the one in use
 */
static int cmd_term(int argc, char **argv)
{
#if CONFIG_CLI_API_TERMINAL_MODE_LINE_EDITING
  const char *configured = "Force line editing";
#elif CONFIG_CLI_API_TERMINAL_MODE_DUMB
  const char *configured = "Force dumb mode";
#else
  const char *configured = "Auto-detect";
#endif
  const bool dumb = linenoiseIsDumbMode();

  printf("Configured mode: %s\n", configured);
  printf("Mode in use:     %s\n", dumb ? "dumb (no escape sequences)" : "line editing");

#if CONFIG_CLI_API_TERMINAL_MODE_AUTO
  if (dumb)
    printf("\nYour terminal did not answer the probe at boot. If it supports VT100\n"
           "escape sequences, select 'Force line editing' in menuconfig >\n"
           "CLI API Configuration > Terminal mode.\n");
#endif

  return 0;
}

/* ========================================================================== */
/*                  COUNTER (shared prefix for TAB completion)                */
/* ========================================================================== */

static int s_counter = 0;

/**
 * @brief Adds a value to the counter.
 *
 * Usage: counter-add -n <value>
 */
static int cmd_counter_add(cli_context_t *ctx)
{
  s_counter += ctx->args[0].int_value;
  printf("Counter: %d\n", s_counter);
  return 0;
}

static const cli_command_t counter_add_cmd = {
  .name = "counter-add",
  .description = "Adds a value to the counter",
  .hint = NULL, /* NULL: esp_console builds the hint from the arguments */
  .callback = cmd_counter_add,
  .args =
    {
      {.short_opt = "n",
       .long_opt = "value",
       .datatype = "<value>",
       .description = "Value to add (may be negative)",
       .type = CLI_ARG_TYPE_INT,
       .required = true},
    },
  .arg_count = 1,
};

/**
 * @brief "counter-show" command - prints the counter
 */
static int cmd_counter_show(int argc, char **argv)
{
  printf("Counter: %d\n", s_counter);
  return 0;
}

/**
 * @brief "counter-reset" command - sets the counter back to 0
 */
static int cmd_counter_reset(int argc, char **argv)
{
  s_counter = 0;
  printf("Counter: %d\n", s_counter);
  return 0;
}

/* ========================================================================== */
/*                                 MAIN                                       */
/* ========================================================================== */

void app_main(void)
{
  /* Configure and initialize the CLI */
  cli_config_t cli_cfg = {
    .prompt = "term> ",
    .banner = "\n"
              "=== CLI-API Terminal Example ===\n"
              "Try the line editing features:\n"
              "  TAB        complete a command (type 'cou', then TAB again to cycle)\n"
              "  UP/DOWN    browse previous commands\n"
              "  Backspace  erase the last character\n"
              "\n"
              "Commands:\n"
              "  term               show the configured and in-use terminal mode\n"
              "  counter-add -n N   add N to the counter (watch the hint while typing)\n"
              "  counter-show       print the counter\n"
              "  counter-reset      reset the counter\n"
              "  history [-c]       list (or clear) the command history\n"
              "  sync               save the history to flash\n"
              "================================",
    .register_help = true,
    .history_sync = true, /* registers 'history' and 'sync' */
  };

  ESP_ERROR_CHECK(cli_init(&cli_cfg));

  cli_register_simple_command("term", "Shows the configured and the in-use terminal mode", cmd_term);
  ESP_ERROR_CHECK(cli_register_command(&counter_add_cmd));
  cli_register_simple_command("counter-show", "Prints the counter", cmd_counter_show);
  cli_register_simple_command("counter-reset", "Resets the counter to 0", cmd_counter_reset);

  ESP_LOGI(TAG, "Terminal example commands registered: term, counter-add, counter-show, counter-reset");

  /* Start the console loop (blocks here) */
  cli_run();

  /* Finalize (only reached on exit) */
  cli_deinit();

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
