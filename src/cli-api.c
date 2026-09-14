/**
 * @file cli-api.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief Lifecycle orchestration: cli_init(), cli_run(), cli_deinit(), cli_get_prompt().
 *
 * Wires together storage (cli-api-storage.c), peripheral bring-up
 * (cli-api-peripheral.c) and console setup (cli-api-console.c).
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "cli-api-private.h"

#include <esp_console.h>
#include <esp_log.h>
#include <linenoise/linenoise.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "cli-api";

esp_err_t cli_init(const cli_config_t *config)
{
  if (s_cli.initialized)
  {
    ESP_LOGW(TAG, "CLI already initialized");
    return ESP_OK;
  }

  /* Use default configuration if none provided */
  cli_config_t default_config = CLI_CONFIG_DEFAULT();
  if (config == NULL)
    config = &default_config;

  /* Initialize NVS */
  esp_err_t err = cli_init_nvs();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize NVS");
    return err;
  }

  /* Initialize filesystem if history enabled */
  if (config->store_history)
  {
    esp_err_t err = cli_init_filesystem();
    if (err != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to mount filesystem, history disabled");
      s_cli.store_history = false;
    }
    else
      s_cli.store_history = true;
  }
  else
    s_cli.store_history = false;

  cli_init_peripheral();
  cli_init_linenoise();

  cli_setup_prompt(config->prompt);

  if (config->register_help)
    esp_console_register_help_command();

  if (config->banner != NULL)
    printf("\n%s\n", config->banner);
  else
    printf("\n"
           "ESP32 CLI Console\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n\n");

  if (linenoiseIsDumbMode())
    printf("Terminal does not support escape sequences.\n"
           "Line editing and history features are disabled.\n\n");

  s_cli.initialized = true;
  ESP_LOGI(TAG, "CLI successfully initialized");

  return ESP_OK;
}

esp_err_t cli_run(void)
{
  if (!s_cli.initialized)
  {
    ESP_LOGE(TAG, "CLI not initialized. Call cli_init() first.");
    return ESP_ERR_INVALID_STATE;
  }

  while (true)
  {
    /* Read line from user */
    char *line = linenoise(s_cli.prompt);

    if (line == NULL)
    {
#if CONFIG_CLI_API_IGNORE_EMPTY_LINES
      continue;
#else
      break;
#endif
    }

    if (strlen(line) > 0)
    {
      linenoiseHistoryAdd(line);
      if (s_cli.store_history)
        linenoiseHistorySave(CLI_HISTORY_PATH);
    }

    /* Execute the command */
    int ret;
    esp_err_t err = esp_console_run(line, &ret);

    if (err == ESP_ERR_NOT_FOUND)
      printf("Command not recognized\n");
    else if (err == ESP_OK && ret != ESP_OK)
      printf("Command returned error: 0x%x (%s)\n", ret, esp_err_to_name(ret));
    else if (err != ESP_OK)
      printf("Internal error: %s\n", esp_err_to_name(err));
    // Empty command, ignore
    // if (err == ESP_ERR_INVALID_ARG)

    linenoiseFree(line);
  }

  ESP_LOGE(TAG, "Console terminated");
  return ESP_OK;
}

void cli_deinit(void)
{
  if (s_cli.initialized)
  {
    esp_console_deinit();

    if (s_cli.store_history)
    {
      cli_deinit_filesystem();
      s_cli.store_history = false;
    }

    s_cli.initialized = false;
    s_cli.cmd_count = 0;
    ESP_LOGI(TAG, "CLI finalized");
  }
}

const char *cli_get_prompt(void)
{
  return s_cli.prompt;
}
