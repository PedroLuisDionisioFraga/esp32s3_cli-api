/**
 * @file cli-api.c
 * @brief Implementation of simplified API for ESP-IDF console commands
 */

#include "cli-api.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "linenoise/linenoise.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

/* Includes for console peripherals */
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#endif
#if CONFIG_ESP_CONSOLE_USB_CDC
#include "esp_vfs_cdcacm.h"
#endif

/* Includes for NVS and FATFS */
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "cli-api";

/* ========================================================================== */
/*                           INTERNAL CONSTANTS                               */
/* ========================================================================== */

#define CLI_MOUNT_PATH   "/data"
#define CLI_HISTORY_PATH CLI_MOUNT_PATH "/history.txt"

/* ========================================================================== */
/*                           INTERNAL VARIABLES                               */
/* ========================================================================== */

/** @brief Console prompt */
static char s_prompt[CLI_PROMPT_MAX_LEN] = "esp> ";

/** @brief Flag indicating if console was initialized */
static bool s_initialized = false;

/** @brief Flag indicating if history is enabled */
static bool s_store_history = false;

/** @brief Wear levelling handle (for FATFS) */
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

/**
 * @brief Internal structure to store registered command data
 */
typedef struct
{
  const cli_command_t *cmd_def;     /**< Original command definition */
  void *argtable[CLI_MAX_ARGS + 1]; /**< Pointers to argtable3 (+1 for arg_end) */
  uint8_t arg_count;                /**< Number of arguments */
} cli_registered_cmd_t;

/** @brief Array of registered commands */
static cli_registered_cmd_t s_registered_cmds[CLI_MAX_COMMANDS];

/** @brief Counter of registered commands */
static uint8_t s_cmd_count = 0;

/* ========================================================================== */
/*                         NVS AND FATFS INITIALIZATION                       */
/* ========================================================================== */

/**
 * @brief Initialize NVS (Non-Volatile Storage)
 */
static esp_err_t cli_init_nvs(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGW(TAG, "NVS partition truncated, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "NVS initialized");
  }
  return err;
}

/**
 * @brief Initialize FATFS filesystem to store history
 */
static esp_err_t cli_init_filesystem(void)
{
  const esp_vfs_fat_mount_config_t mount_config = {.max_files = 4,
                                                   .format_if_mount_failed = true,
                                                   .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};

  esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(CLI_MOUNT_PATH, "storage", &mount_config, &s_wl_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to mount FATFS (%s). History disabled.", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "FATFS mounted at %s", CLI_MOUNT_PATH);
  return ESP_OK;
}

/**
 * @brief Unmount FATFS filesystem
 */
static void cli_deinit_filesystem(void)
{
  if (s_wl_handle != WL_INVALID_HANDLE)
  {
    esp_vfs_fat_spiflash_unmount_rw_wl(CLI_MOUNT_PATH, s_wl_handle);
    s_wl_handle = WL_INVALID_HANDLE;
    ESP_LOGI(TAG, "FATFS unmounted");
  }
}

/* ========================================================================== */
/*                    PERIPHERAL INITIALIZATION                               */
/* ========================================================================== */

/**
 * @brief Initialize console peripheral (UART, USB CDC or USB Serial JTAG)
 */
static void cli_init_peripheral(void)
{
  /* Drain stdout before reconfiguring */
  fflush(stdout);
  fsync(fileno(stdout));

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  /* UART: Configure line endings */
  uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
  uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

  /* Configure UART */
  const uart_config_t uart_config = {
    .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
    .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
    .source_clk = UART_SCLK_XTAL,
#endif
  };
  ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
  uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
  /* USB CDC: Configure line endings */
  esp_vfs_dev_cdcacm_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  esp_vfs_dev_cdcacm_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  /* USB Serial JTAG: Configure line endings */
  usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

  usb_serial_jtag_driver_config_t jtag_config = {
    .tx_buffer_size = 256,
    .rx_buffer_size = 256,
  };
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&jtag_config));
  usb_serial_jtag_vfs_use_driver();

#else
#error Unsupported console type
#endif

  /* Disable buffering on stdin */
  setvbuf(stdin, NULL, _IONBF, 0);
}

/**
 * @brief Initialize linenoise library and esp_console
 */
static void cli_init_linenoise(void)
{
  /* Initialize esp_console */
  esp_console_config_t console_config = {.max_cmdline_args = CLI_MAX_ARGS,
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

  /* Load history if configured */
  if (s_store_history)
  {
    linenoiseHistoryLoad(CLI_HISTORY_PATH);
  }

  /* Detect escape sequences support */
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  /* USB Serial JTAG: skip detection, assume smart terminal */
  linenoiseSetDumbMode(0);
#else
  const int probe_status = linenoiseProbe();
  if (probe_status)
  {
    linenoiseSetDumbMode(1);
  }
#endif
}

/**
 * @brief Configure console prompt
 */
static void cli_setup_prompt(const char *prompt_str)
{
  const char *prompt_temp = "esp> ";
  if (prompt_str != NULL)
  {
    prompt_temp = prompt_str;
  }

#if CONFIG_LOG_COLORS
  if (!linenoiseIsDumbMode())
  {
    snprintf(s_prompt, CLI_PROMPT_MAX_LEN - 1, LOG_COLOR_I "%s" LOG_RESET_COLOR, prompt_temp);
  }
  else
  {
    snprintf(s_prompt, CLI_PROMPT_MAX_LEN - 1, "%s", prompt_temp);
  }
#else
  snprintf(s_prompt, CLI_PROMPT_MAX_LEN - 1, "%s", prompt_temp);
#endif
}

/* ========================================================================== */
/*                          FUNCOES PUBLICAS                                  */
/* ========================================================================== */

esp_err_t cli_init(const cli_config_t *config)
{
  if (s_initialized)
  {
    ESP_LOGW(TAG, "CLI ja inicializado");
    return ESP_OK;
  }

  /* Usa configuracao padrao se nao fornecida */
  cli_config_t default_config = CLI_CONFIG_DEFAULT();
  if (config == NULL)
  {
    config = &default_config;
  }

  /* Inicializa NVS */
  esp_err_t err = cli_init_nvs();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Falha ao inicializar NVS");
    return err;
  }

  /* Inicializa filesystem se historico habilitado */
  if (config->store_history)
  {
    esp_err_t err = cli_init_filesystem();
    if (err != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to mount filesystem, history disabled");
      s_store_history = false;
    }
    else
    {
      s_store_history = true;
    }
  }
  else
  {
    s_store_history = false;
  }

  /* Initialize peripheral and linenoise */
  cli_init_peripheral();
  cli_init_linenoise();

  /* Configure prompt */
  cli_setup_prompt(config->prompt);

  /* Register help command if requested */
  if (config->register_help)
  {
    esp_console_register_help_command();
  }

  /* Display banner */
  if (config->banner != NULL)
  {
    printf("\n%s\n", config->banner);
  }
  else
  {
    printf("\n"
           "ESP32 CLI Console\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n\n");
  }

  if (linenoiseIsDumbMode())
  {
    printf("Terminal does not support escape sequences.\n"
           "Line editing and history features are disabled.\n\n");
  }

  s_initialized = true;
  ESP_LOGI(TAG, "CLI successfully initialized");

  return ESP_OK;
}

esp_err_t cli_run(void)
{
  if (!s_initialized)
  {
    ESP_LOGE(TAG, "CLI not initialized. Call cli_init() first.");
    return ESP_ERR_INVALID_STATE;
  }

  while (true)
  {
    /* Read line from user */
    char *line = linenoise(s_prompt);

    if (line == NULL)
    {
#if CONFIG_CONSOLE_IGNORE_EMPTY_LINES
      continue;
#else
      break;
#endif
    }

    /* Add to history if not empty */
    if (strlen(line) > 0)
    {
      linenoiseHistoryAdd(line);
      if (s_store_history)
      {
        linenoiseHistorySave(CLI_HISTORY_PATH);
      }
    }

    /* Execute the command */
    int ret;
    esp_err_t err = esp_console_run(line, &ret);

    if (err == ESP_ERR_NOT_FOUND)
    {
      printf("Command not recognized\n");
    }
    else if (err == ESP_ERR_INVALID_ARG)
    {
      /* Empty command, ignore */
    }
    else if (err == ESP_OK && ret != ESP_OK)
    {
      printf("Command returned error: 0x%x (%s)\n", ret, esp_err_to_name(ret));
    }
    else if (err != ESP_OK)
    {
      printf("Internal error: %s\n", esp_err_to_name(err));
    }

    /* Free memory allocated by linenoise */
    linenoiseFree(line);
  }

  ESP_LOGE(TAG, "Console terminated");
  return ESP_OK;
}

void cli_deinit(void)
{
  if (s_initialized)
  {
    esp_console_deinit();

    /* Unmount filesystem if it was mounted */
    if (s_store_history)
    {
      cli_deinit_filesystem();
      s_store_history = false;
    }

    s_initialized = false;
    s_cmd_count = 0;
    ESP_LOGI(TAG, "CLI finalized");
  }
}

const char *cli_get_prompt(void)
{
  return s_prompt;
}

/* ========================================================================== */
/*                       COMMAND REGISTRATION                                 */
/* ========================================================================== */

/**
 * @brief Wrapper callback that parses arguments and calls user callback
 */
static int cli_command_wrapper(int argc, char **argv)
{
  /* Find registered command by name (argv[0]) */
  cli_registered_cmd_t *reg_cmd = NULL;
  for (int i = 0; i < s_cmd_count; i++)
  {
    if (strcmp(s_registered_cmds[i].cmd_def->name, argv[0]) == 0)
    {
      reg_cmd = &s_registered_cmds[i];
      break;
    }
  }

  if (reg_cmd == NULL)
  {
    ESP_LOGE(TAG, "Command '%s' not found internally", argv[0]);
    return 1;
  }

  const cli_command_t *cmd = reg_cmd->cmd_def;

  /* Parse arguments using argtable3 */
  int nerrors = arg_parse(argc, argv, reg_cmd->argtable);

  /* If there are parsing errors, show and return */
  if (nerrors != 0)
  {
    struct arg_end *end = reg_cmd->argtable[cmd->arg_count];
    arg_print_errors(stderr, end, argv[0]);
    return 1;
  }

  /* Prepara o contexto para o callback do usuario */
  cli_context_t ctx = {
    .argc = argc,
    .argv = argv,
    .arg_count = cmd->arg_count,
  };

  /* Extrai valores dos argumentos parseados */
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

  /* Chama o callback do usuario */
  return cmd->callback(&ctx);
}

esp_err_t cli_register_command(const cli_command_t *cmd)
{
  if (cmd == NULL || cmd->name == NULL || cmd->callback == NULL)
  {
    ESP_LOGE(TAG, "Parametros invalidos");
    return ESP_ERR_INVALID_ARG;
  }

  if (s_cmd_count >= CLI_MAX_COMMANDS)
  {
    ESP_LOGE(TAG, "Limite de comandos atingido (%d)", CLI_MAX_COMMANDS);
    return ESP_ERR_NO_MEM;
  }

  /* Se nao tem argumentos, usa registro simples */
  if (cmd->arg_count == 0)
  {
    const esp_console_cmd_t esp_cmd = {
      .command = cmd->name,
      .help = cmd->description,
      .hint = cmd->hint,
      .func = (int (*)(int, char **))cli_command_wrapper,
    };

    /* Salva referencia */
    s_registered_cmds[s_cmd_count].cmd_def = cmd;
    s_registered_cmds[s_cmd_count].arg_count = 0;
    s_cmd_count++;

    return esp_console_cmd_register(&esp_cmd);
  }

  /* Aloca estruturas argtable3 para cada argumento */
  cli_registered_cmd_t *reg_cmd = &s_registered_cmds[s_cmd_count];
  reg_cmd->cmd_def = cmd;
  reg_cmd->arg_count = cmd->arg_count;

  for (int i = 0; i < cmd->arg_count; i++)
  {
    const cli_arg_t *arg = &cmd->args[i];

    switch (arg->type)
    {
      case CLI_ARG_TYPE_INT:
        if (arg->required)
        {
          reg_cmd->argtable[i] = arg_int1(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        }
        else
        {
          reg_cmd->argtable[i] = arg_int0(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        }
        break;

      case CLI_ARG_TYPE_STRING:
        if (arg->required)
        {
          reg_cmd->argtable[i] = arg_str1(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        }
        else
        {
          reg_cmd->argtable[i] = arg_str0(arg->short_opt, arg->long_opt, arg->datatype, arg->description);
        }
        break;

      case CLI_ARG_TYPE_FLAG:
        if (arg->required)
        {
          reg_cmd->argtable[i] = arg_lit1(arg->short_opt, arg->long_opt, arg->description);
        }
        else
        {
          reg_cmd->argtable[i] = arg_lit0(arg->short_opt, arg->long_opt, arg->description);
        }
        break;

      default:
        ESP_LOGE(TAG, "Invalid argument type: %d", arg->type);
        return ESP_ERR_INVALID_ARG;
    }

    if (reg_cmd->argtable[i] == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate argument %d", i);
      for (int j = 0; j < i; j++)
      {
        free(reg_cmd->argtable[j]);
      }
      return ESP_ERR_NO_MEM;
    }
  }

  /* Add arg_end at the end */
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

  s_cmd_count++;
  ESP_LOGI(TAG, "Comando '%s' registrado com %d argumentos", cmd->name, cmd->arg_count);

  return ESP_OK;
}

esp_err_t cli_register_simple_command(const char *name, const char *description, int (*callback)(int argc, char **argv))
{
  if (name == NULL || callback == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  const esp_console_cmd_t cmd = {
    .command = name,
    .help = description,
    .hint = NULL,
    .func = callback,
  };

  esp_err_t ret = esp_console_cmd_register(&cmd);
  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "Comando simples '%s' registrado", name);
  }

  return ret;
}

esp_err_t cli_register_commands(const cli_command_t *commands, size_t count)
{
  if (commands == NULL || count == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  for (size_t i = 0; i < count; i++)
  {
    esp_err_t ret = cli_register_command(&commands[i]);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Falha ao registrar comando %zu: %s", i, esp_err_to_name(ret));
      return ret;
    }
  }

  return ESP_OK;
}
