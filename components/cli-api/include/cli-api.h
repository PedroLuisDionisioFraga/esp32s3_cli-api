/**
 * @file cli-api.h
 * @brief Simplified API for creating console commands in ESP-IDF
 *
 * This API abstracts the complexity of esp_console and argtable3, allowing
 * commands to be registered simply using a descriptive struct.
 * Also manages console initialization and main loop.
 *
 * @author Pedro Luis Dionisio Fraga
 * @date 2026
 */

#ifndef CLI_API_H
#define CLI_API_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* ========================================================================== */
/*                              CONFIGURATION                                 */
/* ========================================================================== */

/**
 * @brief Maximum number of arguments per command
 */
#define CLI_MAX_ARGS 8

/**
 * @brief Maximum number of registered commands
 */
#define CLI_MAX_COMMANDS 32

/**
 * @brief Maximum command line length
 */
#define CLI_MAX_CMDLINE_LENGTH 256

/**
 * @brief Maximum prompt size
 */
#define CLI_PROMPT_MAX_LEN 64

/**
 * @brief Command history size
 */
#define CLI_HISTORY_SIZE 100

/* ========================================================================== */
/*                           TYPES AND STRUCTURES                             */
/* ========================================================================== */

/**
 * @brief Supported argument types
 */
typedef enum
{
  CLI_ARG_TYPE_INT,    /**< Integer argument (ex: --timeout 5000) */
  CLI_ARG_TYPE_STRING, /**< String argument (ex: --ssid "MyNetwork") */
  CLI_ARG_TYPE_FLAG,   /**< Boolean flag (ex: -v or --verbose) */
} cli_arg_type_t;

/**
 * @brief Command argument descriptor
 */
typedef struct
{
  const char *short_opt;   /**< Short option (ex: "t" for -t), NULL if none */
  const char *long_opt;    /**< Long option (ex: "timeout" for --timeout), NULL if none */
  const char *datatype;    /**< Type displayed in help (ex: "<ms>", "<ssid>") */
  const char *description; /**< Argument description for help */
  cli_arg_type_t type;     /**< Argument type */
  bool required;           /**< true = required, false = optional */
} cli_arg_t;

/**
 * @brief Parsed argument values (passed to callback)
 */
typedef struct
{
  int int_value;         /**< Integer value (valid if type == CLI_ARG_TYPE_INT) */
  const char *str_value; /**< String value (valid if type == CLI_ARG_TYPE_STRING) */
  bool flag_value;       /**< Boolean value (valid if type == CLI_ARG_TYPE_FLAG) */
  int count;             /**< Number of times the argument appeared (0 = not provided) */
} cli_arg_value_t;

/**
 * @brief Context passed to command callback
 */
typedef struct
{
  int argc;                           /**< Number of original arguments */
  char **argv;                        /**< Original arguments */
  cli_arg_value_t args[CLI_MAX_ARGS]; /**< Parsed argument values */
  uint8_t arg_count;                  /**< Number of defined arguments */
} cli_context_t;

/**
 * @brief Command callback type
 *
 * @param ctx Context with parsed arguments
 * @return int 0 for success, non-zero for error
 */
typedef int (*cli_callback_t)(cli_context_t *ctx);

/**
 * @brief Complete CLI command descriptor, like a recipe
 */
typedef struct
{
  const char *name;             /**< Command name (ex: "join", "version") */
  const char *description;      /**< Description displayed in help */
  const char *hint;             /**< Optional hint shown during typing */
  cli_callback_t callback;      /**< Callback function executed when command is called */
  cli_arg_t args[CLI_MAX_ARGS]; /**< Command arguments array */
  uint8_t arg_count;            /**< Number of arguments in args[] */
} cli_command_t;

/**
 * @brief CLI console configuration
 */
typedef struct
{
  const char *prompt; /**< Console prompt (ex: "esp32>"). NULL uses default */
  const char *banner; /**< Welcome message. NULL uses default */
  bool register_help; /**< true = automatically register 'help' command */
  bool store_history; /**< true = save history to filesystem (requires "storage" partition) */
} cli_config_t;

/**
 * @brief Macro to initialize cli_config_t with default values
 */
#define CLI_CONFIG_DEFAULT() \
  {                          \
    .prompt = NULL,          \
    .banner = NULL,          \
    .register_help = true,   \
    .store_history = false,  \
  }

/* ========================================================================== */
/*                       INITIALIZATION FUNCTIONS                             */
/* ========================================================================== */

/**
 * @brief Initialize the complete CLI console
 *
 * This function initializes the console peripheral (UART/USB),
 * the linenoise library and esp_console.
 *
 * @param config Console configuration (can be NULL to use default)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t cli_init(const cli_config_t *config);

/**
 * @brief Start the console main loop
 *
 * This function blocks and processes commands continuously.
 * Returns only on error or Ctrl+C.
 *
 * @return esp_err_t ESP_OK if terminated normally
 */
esp_err_t cli_run(void);

/**
 * @brief Finalize the CLI console
 *
 * Frees resources allocated by the console.
 */
void cli_deinit(void);

/**
 * @brief Returns the current prompt
 *
 * @return const char* Pointer to the prompt
 */
const char *cli_get_prompt(void);

/* ========================================================================== */
/*                       COMMAND REGISTRATION FUNCTIONS                       */
/* ========================================================================== */

/**
 * @brief Register a command in the console
 *
 * @param cmd Pointer to struct describing the command
 * @return esp_err_t
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_ARG: Invalid parameter
 *         - ESP_ERR_NO_MEM: No memory to allocate argtable
 */
esp_err_t cli_register_command(const cli_command_t *cmd);

/**
 * @brief Register a simple command (no arguments)
 *
 * Shortcut for commands that don't need arguments.
 *
 * @param name Command name
 * @param description Description for help
 * @param callback Callback function (receives standard argc/argv)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t cli_register_simple_command(const char *name, const char *description,
                                      int (*callback)(int argc, char **argv));

/**
 * @brief Register multiple commands at once
 *
 * @param commands Array of commands
 * @param count Number of commands in the array
 * @return esp_err_t ESP_OK if all registered successfully
 */
esp_err_t cli_register_commands(const cli_command_t *commands, size_t count);

#endif /* CLI_API_H */
