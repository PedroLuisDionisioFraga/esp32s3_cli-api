/*
 * SPDX-FileCopyrightText: 2026 Pedro Luis Dionisio Fraga
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/**
 * @file basic_example_main.c
 * @brief Basic CLI-API example using simple commands (no arguments)
 *
 * This example demonstrates the simplest way to use CLI-API:
 * registering commands with cli_register_simple_command() that
 * receive standard (int argc, char **argv) parameters.
 */

#include <stdio.h>
#include <string.h>

#include "cli-api.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "basic_example";

/* ========================================================================== */
/*                           SIMPLE COMMANDS                                  */
/* ========================================================================== */

/**
 * @brief "hello" command - prints a greeting
 */
static int cmd_hello(int argc, char **argv)
{
  printf("Hello World! Welcome to ESP32 console!\n");
  return 0;
}

/**
 * @brief "status" command - prints system status
 */
static int cmd_status(int argc, char **argv)
{
  printf("+--------------------------+\n");
  printf("|     System Status        |\n");
  printf("+--------------------------+\n");
  printf("|  Free heap:  %6lu B    |\n", (unsigned long)esp_get_free_heap_size());
  printf("|  Min heap:   %6lu B    |\n", (unsigned long)esp_get_minimum_free_heap_size());
  printf("|  IDF ver:    %-11s |\n", esp_get_idf_version());
  printf("+--------------------------+\n");
  return 0;
}

/**
 * @brief "about" command - prints project info
 */
static int cmd_about(int argc, char **argv)
{
  printf("CLI-API Basic Example\n");
  printf("  A simplified API for ESP-IDF console commands.\n");
  printf("  See: https://github.com/PedroLuisDionisioFraga/esp32s3_cli-api\n");
  return 0;
}

/* ========================================================================== */
/*                                 MAIN                                       */
/* ========================================================================== */

void app_main(void)
{
  /* Configure and initialize the CLI */
  cli_config_t cli_cfg = {
    .prompt = "basic> ",
    .banner = "\n"
              "=== CLI-API Basic Example ===\n"
              "Type 'help' to get the list of commands.\n"
              "Use UP/DOWN arrows for command history.\n"
              "Press TAB to auto-complete.\n"
              "=============================",
    .register_help = true,
    .store_history = true,
  };

  ESP_ERROR_CHECK(cli_init(&cli_cfg));

  /* Register simple commands (no arguments needed) */
  cli_register_simple_command("hello", "Prints Hello World", cmd_hello);
  cli_register_simple_command("status", "Shows system status (heap, IDF version)", cmd_status);
  cli_register_simple_command("about", "Prints project information", cmd_about);

  ESP_LOGI(TAG, "Basic example commands registered: hello, status, about");

  /* Start the console loop (blocks here) */
  cli_run();

  /* Finalize (only reached on exit) */
  cli_deinit();
}
