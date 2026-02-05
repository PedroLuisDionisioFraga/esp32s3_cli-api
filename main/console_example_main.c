/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/soc_caps.h"
#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"
#include "console_settings.h"
#include "cli-api.h"

/*
 * We warn if a secondary serial console is enabled. A secondary serial console is always output-only and
 * hence not very useful for interactive console applications. If you encounter this warning, consider disabling
 * the secondary serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char* TAG = "example";
#define PROMPT_STR CONFIG_IDF_TARGET

/* ========================================================================== */
/*                    EXEMPLOS DE COMANDOS USANDO CLI-API                     */
/* ========================================================================== */

/**
 * @brief Exemplo 1: Comando simples sem argumentos
 */
static int cmd_hello(int argc, char **argv)
{
    printf("Hello World! Bem-vindo ao console ESP32!\n");
    return 0;
}

/**
 * @brief Exemplo 2: Comando com argumentos usando cli_context_t
 *
 * Uso: echo --msg "sua mensagem" [-n repeticoes] [-u]
 */
static int cmd_echo(cli_context_t *ctx)
{
    /* Argumento 0: mensagem (string obrigatória) */
    const char *msg = ctx->args[0].str_value;

    /* Argumento 1: número de repetições (int opcional, default=1) */
    int repeat = (ctx->args[1].count > 0) ? ctx->args[1].int_value : 1;

    /* Argumento 2: uppercase flag (opcional) */
    bool uppercase = ctx->args[2].flag_value;

    for (int i = 0; i < repeat; i++) {
        if (uppercase) {
            /* Imprime em maiúsculas */
            for (const char *p = msg; *p; p++) {
                printf("%c", (*p >= 'a' && *p <= 'z') ? (*p - 32) : *p);
            }
            printf("\n");
        } else {
            printf("%s\n", msg);
        }
    }

    return 0;
}

static const cli_command_t echo_cmd = {
    .name = "echo",
    .description = "Repete uma mensagem N vezes",
    .hint = NULL,
    .callback = cmd_echo,
    .args = {
        {
            .short_opt = "m",
            .long_opt = "msg",
            .datatype = "<texto>",
            .description = "Mensagem a ser exibida",
            .type = CLI_ARG_TYPE_STRING,
            .required = true
        },
        {
            .short_opt = "n",
            .long_opt = "repeat",
            .datatype = "<N>",
            .description = "Número de repetições (default: 1)",
            .type = CLI_ARG_TYPE_INT,
            .required = false
        },
        {
            .short_opt = "u",
            .long_opt = "uppercase",
            .datatype = NULL,
            .description = "Converte para maiúsculas",
            .type = CLI_ARG_TYPE_FLAG,
            .required = false
        },
    },
    .arg_count = 3,
};

/**
 * @brief Exemplo 3: Comando de cálculo matemático
 *
 * Uso: calc -a <num1> -b <num2> [-v]
 */
static int cmd_calc(cli_context_t *ctx)
{
    int a = ctx->args[0].int_value;
    int b = ctx->args[1].int_value;
    bool verbose = ctx->args[2].flag_value;

    if (verbose) {
        printf("Calculando operações com A=%d e B=%d\n", a, b);
        printf("  Soma:         %d + %d = %d\n", a, b, a + b);
        printf("  Subtração:    %d - %d = %d\n", a, b, a - b);
        printf("  Multiplicação:%d * %d = %d\n", a, b, a * b);
        if (b != 0) {
            printf("  Divisão:      %d / %d = %d\n", a, b, a / b);
        } else {
            printf("  Divisão:      indefinida (B=0)\n");
        }
    } else {
        printf("Soma: %d\n", a + b);
    }

    return 0;
}

static const cli_command_t calc_cmd = {
    .name = "calc",
    .description = "Calculadora simples (soma, sub, mult, div)",
    .hint = NULL,
    .callback = cmd_calc,
    .args = {
        {
            .short_opt = "a",
            .long_opt = NULL,
            .datatype = "<num>",
            .description = "Primeiro número",
            .type = CLI_ARG_TYPE_INT,
            .required = true
        },
        {
            .short_opt = "b",
            .long_opt = NULL,
            .datatype = "<num>",
            .description = "Segundo número",
            .type = CLI_ARG_TYPE_INT,
            .required = true
        },
        {
            .short_opt = "v",
            .long_opt = "verbose",
            .datatype = NULL,
            .description = "Mostra todas as operações",
            .type = CLI_ARG_TYPE_FLAG,
            .required = false
        },
    },
    .arg_count = 3,
};

/**
 * @brief Exemplo 4: Comando complexo de configuração de GPIO
 *
 * Demonstra validação de entrada, múltiplos argumentos opcionais,
 * tratamento de erros e interação com hardware.
 *
 * Uso: gpio --pin <0-48> --mode <in|out|od> [--pull <up|down|none>] [--level <0|1>] [-i] [-s]
 *
 * Exemplos:
 *   gpio --pin 2 --mode out --level 1         # Configura GPIO2 como saída em HIGH
 *   gpio --pin 4 --mode in --pull up -i       # Configura GPIO4 como entrada com pull-up e mostra info
 *   gpio --pin 5 --mode od -s                 # Open-drain, salva no NVS
 */

#include "driver/gpio.h"

/* Estrutura para armazenar configuração atual */
typedef struct {
    int pin;
    gpio_mode_t mode;
    gpio_pull_mode_t pull;
    int level;
    bool configured;
} gpio_config_state_t;

static gpio_config_state_t s_gpio_states[GPIO_NUM_MAX] = {0};

static int cmd_gpio(cli_context_t *ctx)
{
    /* Extrai argumentos */
    int pin = ctx->args[0].int_value;
    const char *mode_str = ctx->args[1].str_value;
    const char *pull_str = (ctx->args[2].count > 0) ? ctx->args[2].str_value : "none";
    bool level_specified = (ctx->args[3].count > 0);
    int level = level_specified ? ctx->args[3].int_value : -1;  /* -1 = não especificado */
    bool show_info = ctx->args[4].flag_value;
    bool save_nvs = ctx->args[5].flag_value;

    /* ========== Validação do PIN ========== */
    if (pin < 0 || pin >= GPIO_NUM_MAX) {
        printf("ERRO: GPIO %d inválido. Use 0-%d\n", pin, GPIO_NUM_MAX - 1);
        return 1;
    }

    /* Verifica pinos reservados (ESP32-S3 específico) */
    const int reserved_pins[] = {19, 20, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    for (size_t i = 0; i < sizeof(reserved_pins)/sizeof(reserved_pins[0]); i++) {
        if (pin == reserved_pins[i]) {
            printf("AVISO: GPIO %d pode ser reservado para flash/PSRAM\n", pin);
        }
    }

    /* ========== Parse do modo ========== */
    gpio_mode_t gpio_mode;
    if (strcmp(mode_str, "in") == 0 || strcmp(mode_str, "input") == 0) {
        gpio_mode = GPIO_MODE_INPUT;
    } else if (strcmp(mode_str, "out") == 0 || strcmp(mode_str, "output") == 0) {
        gpio_mode = GPIO_MODE_OUTPUT;
    } else if (strcmp(mode_str, "od") == 0 || strcmp(mode_str, "open-drain") == 0) {
        gpio_mode = GPIO_MODE_OUTPUT_OD;
    } else if (strcmp(mode_str, "inout") == 0) {
        gpio_mode = GPIO_MODE_INPUT_OUTPUT;
    } else if (strcmp(mode_str, "inout_od") == 0) {
        gpio_mode = GPIO_MODE_INPUT_OUTPUT_OD;
    } else {
        printf("ERRO: Modo '%s' inválido. Use: in, out, od, inout, inout_od\n", mode_str);
        return 1;
    }

    /* ========== Parse do pull ========== */
    gpio_pull_mode_t pull_mode;
    bool enable_pullup = false;
    bool enable_pulldown = false;

    if (strcmp(pull_str, "up") == 0) {
        pull_mode = GPIO_PULLUP_ONLY;
        enable_pullup = true;
    } else if (strcmp(pull_str, "down") == 0) {
        pull_mode = GPIO_PULLDOWN_ONLY;
        enable_pulldown = true;
    } else if (strcmp(pull_str, "both") == 0) {
        pull_mode = GPIO_PULLUP_PULLDOWN;
        enable_pullup = true;
        enable_pulldown = true;
    } else if (strcmp(pull_str, "none") == 0 || strcmp(pull_str, "float") == 0) {
        pull_mode = GPIO_FLOATING;
    } else {
        printf("ERRO: Pull '%s' invalido. Use: up, down, both, none\n", pull_str);
        return 1;
    }

    /* ========== Determina o nivel a usar ========== */
    /* Se o usuario nao especificou -l, mantem o nivel salvo internamente */
    /* NOTA: gpio_get_level() retorna sempre 0 se o pino for somente OUTPUT! */
    if (!level_specified) {
        if (s_gpio_states[pin].configured) {
            /* GPIO ja configurado: usa o nivel salvo internamente */
            level = s_gpio_states[pin].level;
        } else {
            /* Primeira configuracao: usa 0 como padrao */
            level = 0;
        }
    }

    /* ========== Validação do nível ========== */
    if (level != 0 && level != 1) {
        printf("ERRO: Nível deve ser 0 ou 1, recebido: %d\n", level);
        return 1;
    }

    /* ========== Configuração do GPIO ========== */
    printf("\n+-----------------------------------------+\n");
    printf("|       Configurando GPIO %-2d              |\n", pin);
    printf("+-----------------------------------------+\n");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = gpio_mode,
        .pull_up_en = enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = enable_pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        printf("|  ERRO: %-30s |\n", esp_err_to_name(ret));
        printf("+-----------------------------------------+\n");
        return 1;
    }

    /* Define nível se for saída */
    if (gpio_mode == GPIO_MODE_OUTPUT || gpio_mode == GPIO_MODE_OUTPUT_OD ||
        gpio_mode == GPIO_MODE_INPUT_OUTPUT || gpio_mode == GPIO_MODE_INPUT_OUTPUT_OD) {
        gpio_set_level(pin, level);
    }

    /* Salva estado */
    s_gpio_states[pin].pin = pin;
    s_gpio_states[pin].mode = gpio_mode;
    s_gpio_states[pin].pull = pull_mode;
    s_gpio_states[pin].level = level;
    s_gpio_states[pin].configured = true;

    /* Exibe resultado */
    const char *mode_names[] = {"DISABLE", "INPUT", "OUTPUT", "OUTPUT_OD", "INPUT_OUTPUT", "", "INPUT_OUTPUT_OD"};
    const char *pull_names[] = {"PULLUP", "PULLDOWN", "UP+DOWN", "FLOATING"};

    printf("|  Modo:      %-27s |\n", mode_names[gpio_mode]);
    printf("|  Pull:      %-27s |\n", pull_names[pull_mode]);

    if (gpio_mode != GPIO_MODE_INPUT) {
        printf("|  Nivel:     %-27s |\n", level ? "HIGH (1)" : "LOW (0)");
    }

    printf("|  Status:    %-27s |\n", "OK - Configurado");

    /* ========== Mostra informacoes extras ========== */
    if (show_info) {
        printf("+-----------------------------------------+\n");
        printf("|           Informacoes Extras            |\n");
        printf("+-----------------------------------------+\n");

        /* Le nivel atual */
        int current_level = gpio_get_level(pin);
        printf("|  Nivel atual lido:  %d                   |\n", current_level);

        /* Conta GPIOs configurados */
        int configured_count = 0;
        for (int i = 0; i < GPIO_NUM_MAX; i++) {
            if (s_gpio_states[i].configured) configured_count++;
        }
        printf("|  GPIOs configurados: %-18d |\n", configured_count);

        /* Mostra caracteristicas do pino */
        printf("|  Suporta RTC:       %-19s |\n", (pin <= 21) ? "Sim" : "Nao");
        printf("|  Suporta ADC:       %-19s |\n", (pin <= 10) ? "Sim (ADC1)" : (pin <= 20) ? "Sim (ADC2)" : "Nao");
    }

    /* ========== Salva no NVS (simulado) ========== */
    if (save_nvs) {
        printf("+-----------------------------------------+\n");
        printf("|  NVS: Configuracao salva!               |\n");
        ESP_LOGI(TAG, "GPIO %d config saved to NVS (mode=%d, pull=%d, level=%d)",
                 pin, gpio_mode, pull_mode, level);
    }

    printf("+-----------------------------------------+\n\n");

    return 0;
}

static const cli_command_t gpio_cmd = {
    .name = "gpio",
    .description = "Configura um GPIO (modo, pull, nível)",
    .hint = NULL,
    .callback = cmd_gpio,
    .args = {
        {
            .short_opt = "p",
            .long_opt = "pin",
            .datatype = "<0-48>",
            .description = "Número do GPIO",
            .type = CLI_ARG_TYPE_INT,
            .required = true
        },
        {
            .short_opt = "m",
            .long_opt = "mode",
            .datatype = "<in|out|od>",
            .description = "Modo: in, out, od, inout, inout_od",
            .type = CLI_ARG_TYPE_STRING,
            .required = true
        },
        {
            .short_opt = NULL,
            .long_opt = "pull",
            .datatype = "<up|down|none>",
            .description = "Resistor pull: up, down, both, none",
            .type = CLI_ARG_TYPE_STRING,
            .required = false
        },
        {
            .short_opt = "l",
            .long_opt = "level",
            .datatype = "<0|1>",
            .description = "Nível inicial (para saída)",
            .type = CLI_ARG_TYPE_INT,
            .required = false
        },
        {
            .short_opt = "i",
            .long_opt = "info",
            .datatype = NULL,
            .description = "Mostra informações extras do GPIO",
            .type = CLI_ARG_TYPE_FLAG,
            .required = false
        },
        {
            .short_opt = "s",
            .long_opt = "save",
            .datatype = NULL,
            .description = "Salva configuração no NVS",
            .type = CLI_ARG_TYPE_FLAG,
            .required = false
        },
    },
    .arg_count = 6,
};

/**
 * @brief Registra todos os comandos de exemplo da CLI-API
 */
static void register_cli_api_examples(void)
{
    /* Comando simples */
    cli_register_simple_command("hello", "Imprime Hello World", cmd_hello);

    /* Comandos com argumentos */
    cli_register_command(&echo_cmd);
    cli_register_command(&calc_cmd);
    cli_register_command(&gpio_cmd);

    ESP_LOGI(TAG, "Comandos de exemplo CLI-API registrados: hello, echo, calc, gpio");
}

/* ========================================================================== */

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_CONSOLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#else
#define HISTORY_PATH NULL
#endif // CONFIG_CONSOLE_STORE_HISTORY

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    initialize_nvs();

#if CONFIG_CONSOLE_STORE_HISTORY
    initialize_filesystem();
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif

    /* Initialize console output periheral (UART, USB_OTG, USB_JTAG) */
    initialize_console_peripheral();

    /* Initialize linenoise library and esp_console*/
    initialize_console_library(HISTORY_PATH);

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char *prompt = setup_prompt(PROMPT_STR ">");

    /* Register commands */
    esp_console_register_help_command();
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

    /* Registra comandos de exemplo usando a CLI-API */
    register_cli_api_examples();

    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n"
           "Ctrl+C will terminate the console environment.\n"
           "\n"
           "=== CLI-API Examples ===\n"
           "  hello              - Imprime Hello World\n"
           "  echo -m <msg>      - Repete mensagem (use -n N, -u)\n"
           "  calc -a N -b M     - Calculadora (use -v para verbose)\n"
           "  gpio -p N -m MODE  - Configura GPIO (use --pull, -l, -i, -s)\n"
           "========================\n");

    if (linenoiseIsDumbMode()) {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Windows Terminal or Putty instead.\n");
    }

    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);

#if CONFIG_CONSOLE_IGNORE_EMPTY_LINES
        if (line == NULL) { /* Ignore empty lines */
            continue;;
        }
#else
        if (line == NULL) { /* Break on EOF or error */
            break;
        }
#endif // CONFIG_CONSOLE_IGNORE_EMPTY_LINES

        /* Add the command to the history if not empty*/
        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
#if CONFIG_CONSOLE_STORE_HISTORY
            /* Save command history to filesystem */
            linenoiseHistorySave(HISTORY_PATH);
#endif // CONFIG_CONSOLE_STORE_HISTORY
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}
