/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

/* Component commands */
#include "cmd_nvs.h"
#include "cmd_system.h"
#include "cmd_wifi.h"

/* CLI-API */
#include "cli-api.h"

#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char *TAG = "example";

/* ========================================================================== */
/*                    EXAMPLES OF COMMANDS USING CLI-API                      */
/* ========================================================================== */

/**
 * @brief Example 1: Simple command without arguments
 */
static int cmd_hello(int argc, char **argv)
{
  printf("Hello World! Welcome to ESP32 console!\n");
  return 0;
}

/**
 * @brief Example 2: Command with arguments using cli_context_t
 *
 * Usage: echo --msg "your message" [-n repetitions] [-u]
 */
static int cmd_echo(cli_context_t *ctx)
{
  /* Argument 0: message (required string) */
  const char *msg = ctx->args[0].str_value;

  /* Argument 1: number of repetitions (optional int, default=1) */
  int repeat = (ctx->args[1].count > 0) ? ctx->args[1].int_value : 1;

  /* Argument 2: uppercase flag (optional) */
  bool uppercase = ctx->args[2].flag_value;

  for (int i = 0; i < repeat; i++)
  {
    if (uppercase)
    {
      /* Print in uppercase */
      for (const char *p = msg; *p; p++)
      {
        printf("%c", (*p >= 'a' && *p <= 'z') ? (*p - 32) : *p);
      }
      printf("\n");
    }
    else
    {
      printf("%s\n", msg);
    }
  }

  return 0;
}

static const cli_command_t echo_cmd = {
  .name = "echo",
  .description = "Repeats a message N times",
  .hint = NULL,
  .callback = cmd_echo,
  .args =
    {
      {.short_opt = "m",
       .long_opt = "msg",
       .datatype = "<text>",
       .description = "Message to be displayed",
       .type = CLI_ARG_TYPE_STRING,
       .required = true},
      {.short_opt = "n",
       .long_opt = "repeat",
       .datatype = "<N>",
       .description = "Number of repetitions (default: 1)",
       .type = CLI_ARG_TYPE_INT,
       .required = false},
      {.short_opt = "u",
       .long_opt = "uppercase",
       .datatype = NULL,
       .description = "Converts to uppercase",
       .type = CLI_ARG_TYPE_FLAG,
       .required = false},
    },
  .arg_count = 3,
};

/**
 * @brief Example 3: Math calculation command
 *
 * Usage: calc -a <num1> -b <num2> [-v]
 */
static int cmd_calc(cli_context_t *ctx)
{
  int a = ctx->args[0].int_value;
  int b = ctx->args[1].int_value;
  bool verbose = ctx->args[2].flag_value;

  if (verbose)
  {
    printf("Calculating operations with A=%d and B=%d\n", a, b);
    printf("  Addition:        %d + %d = %d\n", a, b, a + b);
    printf("  Subtraction:     %d - %d = %d\n", a, b, a - b);
    printf("  Multiplication:  %d * %d = %d\n", a, b, a * b);
    if (b != 0)
    {
      printf("  Division:        %d / %d = %d\n", a, b, a / b);
    }
    else
    {
      printf("  Division:        undefined (B=0)\n");
    }
  }
  else
  {
    printf("Sum: %d\n", a + b);
  }

  return 0;
}

static const cli_command_t calc_cmd = {
  .name = "calc",
  .description = "Simple calculator (addition, subtraction, multiplication, division)",
  .hint = NULL,
  .callback = cmd_calc,
  .args =
    {
      {.short_opt = "a",
       .long_opt = NULL,
       .datatype = "<num>",
       .description = "First number",
       .type = CLI_ARG_TYPE_INT,
       .required = true},
      {.short_opt = "b",
       .long_opt = NULL,
       .datatype = "<num>",
       .description = "Second number",
       .type = CLI_ARG_TYPE_INT,
       .required = true},
      {.short_opt = "v",
       .long_opt = "verbose",
       .datatype = NULL,
       .description = "Shows all operations",
       .type = CLI_ARG_TYPE_FLAG,
       .required = false},
    },
  .arg_count = 3,
};

/**
 * @brief Example 4: Complex GPIO configuration command
 *
 * Demonstrates input validation, multiple optional arguments,
 * error handling and hardware interaction.
 *
 * Usage: gpio --pin <0-48> --mode <in|out|od> [--pull <up|down|none>] [--level
 * <0|1>] [-i] [-s]
 *
 * Examples:
 *   gpio --pin 2 --mode out --level 1         # Configure GPIO2 as output at HIGH
 *   gpio --pin 4 --mode in --pull up -i       # Configure GPIO4 as input with
 *                                               pull-up and show info
 *   gpio --pin 5 --mode od -s                 # Open-drain, save to NVS
 */

#include "driver/gpio.h"

/* Structure to store current GPIO configuration */
typedef struct
{
  int pin;
  gpio_mode_t mode;
  gpio_pull_mode_t pull;
  int level;
  bool configured;
} gpio_config_state_t;

static gpio_config_state_t s_gpio_states[GPIO_NUM_MAX] = {0};

static int cmd_gpio(cli_context_t *ctx)
{
  /* Extract arguments */
  int pin = ctx->args[0].int_value;
  const char *mode_str = ctx->args[1].str_value;
  const char *pull_str = (ctx->args[2].count > 0) ? ctx->args[2].str_value : "none";
  bool level_specified = (ctx->args[3].count > 0);
  int level = level_specified ? ctx->args[3].int_value : -1; /* -1 = not specified */
  bool show_info = ctx->args[4].flag_value;
  bool save_nvs = ctx->args[5].flag_value;

  /* ========== PIN Validation ========== */
  if (pin < 0 || pin >= GPIO_NUM_MAX)
  {
    printf("ERROR: GPIO %d invalid. Use 0-%d\n", pin, GPIO_NUM_MAX - 1);
    return 1;
  }

  /* Check reserved pins (ESP32-S3 specific) */
  const int reserved_pins[] = {19, 20, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
  for (size_t i = 0; i < sizeof(reserved_pins) / sizeof(reserved_pins[0]); i++)
  {
    if (pin == reserved_pins[i])
    {
      printf("WARNING: GPIO %d may be reserved for flash/PSRAM\n", pin);
    }
  }

  /* ========== Parse mode ========== */
  gpio_mode_t gpio_mode;
  if (strcmp(mode_str, "in") == 0 || strcmp(mode_str, "input") == 0)
  {
    gpio_mode = GPIO_MODE_INPUT;
  }
  else if (strcmp(mode_str, "out") == 0 || strcmp(mode_str, "output") == 0)
  {
    gpio_mode = GPIO_MODE_OUTPUT;
  }
  else if (strcmp(mode_str, "od") == 0 || strcmp(mode_str, "open-drain") == 0)
  {
    gpio_mode = GPIO_MODE_OUTPUT_OD;
  }
  else if (strcmp(mode_str, "inout") == 0)
  {
    gpio_mode = GPIO_MODE_INPUT_OUTPUT;
  }
  else if (strcmp(mode_str, "inout_od") == 0)
  {
    gpio_mode = GPIO_MODE_INPUT_OUTPUT_OD;
  }
  else
  {
    printf("ERROR: Mode '%s' invalid. Use: in, out, od, inout, inout_od\n", mode_str);
    return 1;
  }

  /* ========== Parse pull ========== */
  gpio_pull_mode_t pull_mode;
  bool enable_pullup = false;
  bool enable_pulldown = false;

  if (strcmp(pull_str, "up") == 0)
  {
    pull_mode = GPIO_PULLUP_ONLY;
    enable_pullup = true;
  }
  else if (strcmp(pull_str, "down") == 0)
  {
    pull_mode = GPIO_PULLDOWN_ONLY;
    enable_pulldown = true;
  }
  else if (strcmp(pull_str, "both") == 0)
  {
    pull_mode = GPIO_PULLUP_PULLDOWN;
    enable_pullup = true;
    enable_pulldown = true;
  }
  else if (strcmp(pull_str, "none") == 0 || strcmp(pull_str, "float") == 0)
  {
    pull_mode = GPIO_FLOATING;
  }
  else
  {
    printf("ERROR: Pull '%s' invalid. Use: up, down, both, none\n", pull_str);
    return 1;
  }

  /* ========== Determine level to use ========== */
  /* If user hasn't specified -l, keep the internally saved level */
  /* NOTE: gpio_get_level() always returns 0 if pin is OUTPUT only! */
  if (!level_specified)
  {
    if (s_gpio_states[pin].configured)
    {
      /* GPIO already configured: use internally saved level */
      level = s_gpio_states[pin].level;
    }
    else
    {
      /* First configuration: use 0 as default */
      level = 0;
    }
  }

  /* ========== Level Validation ========== */
  if (level != 0 && level != 1)
  {
    printf("ERROR: Level must be 0 or 1, received: %d\n", level);
    return 1;
  }

  /* ========== GPIO Configuration ========== */
  printf("\n+-----------------------------------------+\n");
  printf("|       Configuring GPIO %-2d              |\n", pin);
  printf("+-----------------------------------------+\n");

  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << pin),
    .mode = gpio_mode,
    .pull_up_en = enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
    .pull_down_en = enable_pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };

  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK)
  {
    printf("|  ERROR: %-30s |\n", esp_err_to_name(ret));
    printf("+-----------------------------------------+\n");
    return 1;
  }

  /* Set level if output */
  if (gpio_mode == GPIO_MODE_OUTPUT || gpio_mode == GPIO_MODE_OUTPUT_OD || gpio_mode == GPIO_MODE_INPUT_OUTPUT ||
      gpio_mode == GPIO_MODE_INPUT_OUTPUT_OD)
  {
    gpio_set_level(pin, level);
  }

  /* Save state */
  s_gpio_states[pin].pin = pin;
  s_gpio_states[pin].mode = gpio_mode;
  s_gpio_states[pin].pull = pull_mode;
  s_gpio_states[pin].level = level;
  s_gpio_states[pin].configured = true;

  /* Display result */
  const char *mode_names[] = {"DISABLE", "INPUT", "OUTPUT", "OUTPUT_OD", "INPUT_OUTPUT", "", "INPUT_OUTPUT_OD"};
  const char *pull_names[] = {"PULLUP", "PULLDOWN", "UP+DOWN", "FLOATING"};

  printf("|  Mode:      %-27s |\n", mode_names[gpio_mode]);
  printf("|  Pull:      %-27s |\n", pull_names[pull_mode]);

  if (gpio_mode != GPIO_MODE_INPUT)
  {
    printf("|  Level:     %-27s |\n", level ? "HIGH (1)" : "LOW (0)");
  }

  printf("|  Status:    %-27s |\n", "OK - Configured");

  /* ========== Save to NVS (simulated) ========== */
  if (save_nvs)
  {
    printf("+-----------------------------------------+\n");
    printf("|  NVS: Configuration saved!              |\n");
    ESP_LOGI(TAG, "GPIO %d config saved to NVS (mode=%d, pull=%d, level=%d)", pin, gpio_mode, pull_mode, level);
  }

  printf("+-----------------------------------------+\n\n");

  return 0;
}

static const cli_command_t gpio_cmd = {
  .name = "gpio",
  .description = "Configure a GPIO (mode, pull, level)",
  .hint = NULL,
  .callback = cmd_gpio,
  .args =
    {
      {
        .short_opt = "p",
        .long_opt = "pin",
        .datatype = "<0-48>",
        .description = "GPIO number",
        .type = CLI_ARG_TYPE_INT,
        .required = true,
      },
      {
        .short_opt = "m",
        .long_opt = "mode",
        .datatype = "<in|out|od>",
        .description = "Mode: in, out, od, inout, inout_od",
        .type = CLI_ARG_TYPE_STRING,
        .required = true,
      },
      {
        .short_opt = NULL,
        .long_opt = "pull",
        .datatype = "<up|down|none>",
        .description = "Resistor pull: up, down, both, none",
        .type = CLI_ARG_TYPE_STRING,
        .required = false,
      },
      {
        .short_opt = "l",
        .long_opt = "level",
        .datatype = "<0|1>",
        .description = "Initial level (for output)",
        .type = CLI_ARG_TYPE_INT,
        .required = false,
      },
      {
        .short_opt = "i",
        .long_opt = "info",
        .datatype = NULL,
        .description = "Show extra GPIO information",
        .type = CLI_ARG_TYPE_FLAG,
        .required = false,
      },
      {
        .short_opt = "s",
        .long_opt = "save",
        .datatype = NULL,
        .description = "Save configuration to NVS",
        .type = CLI_ARG_TYPE_FLAG,
        .required = false,
      },
    },
  .arg_count = 6,
};

/**
 * @brief Register all CLI-API example commands
 */
static void register_cli_api_examples(void)
{
  /* Simple command */
  cli_register_simple_command("hello", "Prints Hello World", cmd_hello);

  /* Commands with arguments */
  cli_register_command(&echo_cmd);
  cli_register_command(&calc_cmd);
  cli_register_command(&gpio_cmd);

  ESP_LOGI(TAG, "CLI-API example commands registered: hello, echo, calc, gpio");
}

/* ========================================================================== */
/*                                 MAIN                                       */
/* ========================================================================== */

void app_main(void)
{
  /* Configure and initialize the CLI */
  cli_config_t cli_cfg = {
    .prompt = CONFIG_IDF_TARGET "> ",
    .banner = "\n"
              "=== ESP32 CLI-API Demo ===\n"
              "Type 'help' to get the list of commands.\n"
              "Use UP/DOWN arrows for command history.\n"
              "Press TAB to auto-complete.\n"
              "\n"
              "CLI-API Examples:\n"
              "  hello              - Prints Hello World\n"
              "  echo -m <msg>      - Repeats message (use -n N, -u)\n"
              "  calc -a N -b M     - Calculator (use -v for verbose)\n"
              "  gpio -p N -m MODE  - Configure GPIO (use --pull, -l, -i, -s)\n"
              "===========================",
    .register_help = true,
    .store_history = true, /* CLI-API manages FATFS partition for history */
  };

  ESP_ERROR_CHECK(cli_init(&cli_cfg));

  /* Register system commands */
  register_system_common();
#if SOC_LIGHT_SLEEP_SUPPORTED
  register_system_light_sleep();
#endif
#if SOC_DEEP_SLEEP_SUPPORTED
  register_system_deep_sleep();
#endif
#if (CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_HOST_WIFI_ENABLED)
  register_wifi();
#endif
  register_nvs();

  /* Register CLI-API example commands */
  register_cli_api_examples();

  /* Start the console loop (blocks here) */
  cli_run();

  /* Finalize */
  cli_deinit();
}
