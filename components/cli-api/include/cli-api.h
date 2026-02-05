/**
 * @file cli-api.h
 * @brief API simplificada para criar comandos de console no ESP-IDF
 *
 * Esta API abstrai a complexidade do esp_console e argtable3, permitindo
 * registrar comandos de forma simples usando uma struct descritiva.
 *
 * @author Pedro Luis Dionisio Fraga
 * @date 2026
 */

#ifndef CLI_API_H
#define CLI_API_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Número máximo de argumentos por comando
 */
#define CLI_MAX_ARGS 8

/**
 * @brief Número máximo de comandos registrados
 */
#define CLI_MAX_COMMANDS 32

/**
 * @brief Tipos de argumentos suportados
 */
typedef enum {
    CLI_ARG_TYPE_INT,       /**< Argumento inteiro (ex: --timeout 5000) */
    CLI_ARG_TYPE_STRING,    /**< Argumento string (ex: --ssid "MinhaRede") */
    CLI_ARG_TYPE_FLAG,      /**< Flag booleana (ex: -v ou --verbose) */
} cli_arg_type_t;

/**
 * @brief Descritor de um argumento de comando
 */
typedef struct {
    const char *short_opt;      /**< Opção curta (ex: "t" para -t), NULL se não tiver */
    const char *long_opt;       /**< Opção longa (ex: "timeout" para --timeout), NULL se não tiver */
    const char *datatype;       /**< Tipo exibido no help (ex: "<ms>", "<ssid>") */
    const char *description;    /**< Descrição do argumento para o help */
    cli_arg_type_t type;        /**< Tipo do argumento */
    bool required;              /**< true = obrigatório, false = opcional */
} cli_arg_t;

/**
 * @brief Valores dos argumentos parseados (passados ao callback)
 */
typedef struct {
    /** @brief Valor inteiro (válido se type == CLI_ARG_TYPE_INT) */
    int int_value;

    /** @brief Valor string (válido se type == CLI_ARG_TYPE_STRING) */
    const char *str_value;

    /** @brief Valor booleano (válido se type == CLI_ARG_TYPE_FLAG) */
    bool flag_value;

    /** @brief Quantidade de vezes que o argumento apareceu (0 = não informado) */
    int count;
} cli_arg_value_t;

/**
 * @brief Contexto passado ao callback do comando
 */
typedef struct {
    int argc;                               /**< Número de argumentos originais */
    char **argv;                            /**< Argumentos originais */
    cli_arg_value_t args[CLI_MAX_ARGS];     /**< Valores parseados dos argumentos */
    uint8_t arg_count;                      /**< Quantidade de argumentos definidos */
} cli_context_t;

/**
 * @brief Tipo do callback do comando
 *
 * @param ctx Contexto com argumentos parseados
 * @return int 0 para sucesso, valor diferente para erro
 */
typedef int (*cli_callback_t)(cli_context_t *ctx);

/**
 * @brief Descritor completo de um comando CLI
 *
 * Exemplo de uso:
 * @code
 * static int my_cmd_handler(cli_context_t *ctx) {
 *     int timeout = ctx->args[0].int_value;
 *     const char *name = ctx->args[1].str_value;
 *     printf("Timeout: %d, Name: %s\n", timeout, name);
 *     return 0;
 * }
 *
 * static const cli_command_t my_cmd = {
 *     .name = "mycommand",
 *     .description = "Meu comando personalizado",
 *     .hint = NULL,
 *     .callback = my_cmd_handler,
 *     .args = {
 *         { .short_opt = "t", .long_opt = "timeout", .datatype = "<ms>",
 *           .description = "Timeout em ms", .type = CLI_ARG_TYPE_INT, .required = false },
 *         { .short_opt = NULL, .long_opt = NULL, .datatype = "<name>",
 *           .description = "Nome do dispositivo", .type = CLI_ARG_TYPE_STRING, .required = true },
 *     },
 *     .arg_count = 2,
 * };
 *
 * cli_register_command(&my_cmd);
 * @endcode
 */
typedef struct {
    const char *name;           /**< Nome do comando (ex: "join", "version") */
    const char *description;    /**< Descrição exibida no help */
    const char *hint;           /**< Hint opcional mostrado durante digitação */
    cli_callback_t callback;    /**< Função callback executada quando comando é chamado */
    cli_arg_t args[CLI_MAX_ARGS]; /**< Array de argumentos do comando */
    uint8_t arg_count;          /**< Quantidade de argumentos em args[] */
} cli_command_t;

/**
 * @brief Registra um comando no console
 *
 * @param cmd Ponteiro para struct descrevendo o comando
 * @return esp_err_t
 *         - ESP_OK: Sucesso
 *         - ESP_ERR_INVALID_ARG: Parâmetro inválido
 *         - ESP_ERR_NO_MEM: Sem memória para alocar argtable
 */
esp_err_t cli_register_command(const cli_command_t *cmd);

/**
 * @brief Registra um comando simples (sem argumentos)
 *
 * Atalho para comandos que não precisam de argumentos.
 *
 * @param name Nome do comando
 * @param description Descrição para o help
 * @param callback Função callback (recebe argc/argv padrão)
 * @return esp_err_t ESP_OK em sucesso
 */
esp_err_t cli_register_simple_command(const char *name,
                                       const char *description,
                                       int (*callback)(int argc, char **argv));

/**
 * @brief Registra múltiplos comandos de uma vez
 *
 * @param commands Array de comandos
 * @param count Quantidade de comandos no array
 * @return esp_err_t ESP_OK se todos registrados com sucesso
 */
esp_err_t cli_register_commands(const cli_command_t *commands, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* CLI_API_H */
