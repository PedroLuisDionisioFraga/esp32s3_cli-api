/**
 * @file cli-api-command.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief Command registration, argument parsing (argtable3) and dispatch.
 *
 * This is the only file that touches `s_cli.cmds[]` / `s_cli.cmd_count`.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <argtable3/argtable3.h>
#include <esp_console.h>
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include "cli-api-private.h"

static const char *TAG = "cli-api";

/**
 * @brief Wrapper function that is called by esp_console when a command is executed. It looks up the registered command,
 * parses arguments using argtable3, and calls the user-defined callback with a cli_context_t structure.
 *
 * @param argc Number of arguments
 * @param argv Array of argument strings (argv[0] is the command name)
 * @return int
 */
static int cli_command_wrapper(int argc, char **argv)
{
  cli_registered_cmd_t *reg_cmd = NULL;
  for (int i = 0; i < s_cli.cmd_count; i++)
  {
    if (strcmp(s_cli.cmds[i].cmd_def->name, argv[0]) == 0)
    {
      reg_cmd = &s_cli.cmds[i];
      break;
    }
  }

  if (reg_cmd == NULL)
  {
    ESP_LOGE(TAG, "Command '%s' not found internally", argv[0]);
    return 1;
  }

  const cli_command_t *cmd = reg_cmd->cmd_def;

  int nerrors = arg_parse(argc, argv, reg_cmd->argtable);
  if (nerrors != 0)
  {
    struct arg_end *end = reg_cmd->argtable[cmd->arg_count];
    arg_print_errors(stderr, end, argv[0]);
    return 1;
  }

  cli_context_t ctx = {
    .argc = argc,
    .argv = argv,
    .arg_count = cmd->arg_count,
  };

  /* Extract parsed argument values */
  for (int i = 0; i < cmd->arg_count; i++)
  {
    void *arg = reg_cmd->argtable[i];

    switch (cmd->args[i].type)
    {
      case CLI_ARG_TYPE_INT:
      {
        struct arg_int *a = (struct arg_int *)arg;
        ctx.args[i].count = a->count;
        ctx.args[i].int_value = (a->count > 0) ? a->ival[0] : 0;
        break;
      }
      case CLI_ARG_TYPE_STRING:
      {
        struct arg_str *a = (struct arg_str *)arg;
        ctx.args[i].count = a->count;
        ctx.args[i].str_value = (a->count > 0) ? a->sval[0] : NULL;
        break;
      }
      case CLI_ARG_TYPE_FLAG:
      {
        struct arg_lit *a = (struct arg_lit *)arg;
        ctx.args[i].count = a->count;
        ctx.args[i].flag_value = (a->count > 0);
        break;
      }
    }
  }

  return cmd->callback(&ctx);
}

esp_err_t cli_register_command(const cli_command_t *cmd)
{
  if (cmd == NULL || cmd->name == NULL || cmd->callback == NULL)
  {
    ESP_LOGE(TAG, "Invalid parameters");
    return ESP_ERR_INVALID_ARG;
  }

  if (s_cli.cmd_count >= CLI_MAX_COMMANDS)
  {
    ESP_LOGE(TAG, "Command limit reached (%d)", CLI_MAX_COMMANDS);
    return ESP_ERR_NO_MEM;
  }

  /* If no arguments, use simple registration */
  if (cmd->arg_count == 0)
  {
    const esp_console_cmd_t esp_cmd = {
      .command = cmd->name,
      .help = cmd->description,
      .hint = cmd->hint,
      .func = (int (*)(int, char **))cli_command_wrapper,
    };

    s_cli.cmds[s_cli.cmd_count].cmd_def = cmd;
    s_cli.cmds[s_cli.cmd_count].arg_count = 0;
    s_cli.cmd_count++;

    return esp_console_cmd_register(&esp_cmd);
  }

  /* Allocate argtable3 structures for each argument */
  cli_registered_cmd_t *reg_cmd = &s_cli.cmds[s_cli.cmd_count];
  reg_cmd->cmd_def = cmd;
  reg_cmd->arg_count = cmd->arg_count;

  for (int i = 0; i < cmd->arg_count; i++)
  {
    const cli_arg_t *arg = &cmd->args[i];

    switch (arg->type)
    {
      case CLI_ARG_TYPE_INT:
      {
        if (arg->required)
          reg_cmd->argtable[i] = arg_int1(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        else
          reg_cmd->argtable[i] = arg_int0(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        break;
      }
      case CLI_ARG_TYPE_STRING:
      {
        if (arg->required)
          reg_cmd->argtable[i] = arg_str1(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        else
          reg_cmd->argtable[i] = arg_str0(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        break;
      }
      case CLI_ARG_TYPE_FLAG:
      {
        if (arg->required)
          reg_cmd->argtable[i] = arg_lit1(arg->short_opt, arg->long_opt, arg->description);
        else
          reg_cmd->argtable[i] = arg_lit0(arg->short_opt, arg->long_opt, arg->description);
        break;
      }
      default:
      {
        ESP_LOGE(TAG, "Invalid argument type: %d", arg->type);
        return ESP_ERR_INVALID_ARG;
      }
    }

    if (reg_cmd->argtable[i] == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate argument %d", i);
      for (int j = 0; j < i; j++) free(reg_cmd->argtable[j]);

      return ESP_ERR_NO_MEM;
    }
  }

  reg_cmd->argtable[cmd->arg_count] = arg_end(cmd->arg_count + 1);

  /* Register command in esp_console */
  const esp_console_cmd_t esp_cmd = {
    .command = cmd->name,
    .help = cmd->description,
    .hint = cmd->hint,
    .func = cli_command_wrapper,
    .argtable = reg_cmd->argtable,
  };

  esp_err_t ret = esp_console_cmd_register(&esp_cmd);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register command '%s': %s", cmd->name, esp_err_to_name(ret));
    arg_freetable(reg_cmd->argtable, cmd->arg_count + 1);
    return ret;
  }

  s_cli.cmd_count++;
  ESP_LOGI(TAG, "Command '%s' registered with %d arguments", cmd->name, cmd->arg_count);

  return ESP_OK;
}

esp_err_t cli_register_simple_command(const char *name, const char *description, int (*callback)(int argc, char **argv))
{
  if (name == NULL || callback == NULL)
    return ESP_ERR_INVALID_ARG;

  const esp_console_cmd_t cmd = {
    .command = name,
    .help = description,
    .hint = NULL,
    .func = callback,
  };

  esp_err_t ret = esp_console_cmd_register(&cmd);
  if (ret == ESP_OK)
    ESP_LOGI(TAG, "Simple command '%s' registered", name);

  return ret;
}

esp_err_t cli_register_commands(const cli_command_t *commands, size_t count)
{
  if (commands == NULL || count == 0)
    return ESP_ERR_INVALID_ARG;

  for (size_t i = 0; i < count; i++)
  {
    esp_err_t ret = cli_register_command(&commands[i]);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to register command %zu: %s", i, esp_err_to_name(ret));
      return ret;
    }
  }

  return ESP_OK;
}
