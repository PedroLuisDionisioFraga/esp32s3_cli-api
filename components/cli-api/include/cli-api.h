/**
 * @file cli-api.h
 * @brief API simplificada para criar comandos de console no ESP-IDF
 *
 * Esta API abstrai a complexidade do esp_console e argtable3, permitindo
 * registrar comandos de forma simples usando uma struct descritiva.
 * Também gerencia a inicializacao do console e o loop principal.
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

/* ========================================================================== */
/*                              CONFIGURACOES                                 */
/* ========================================================================== */

/**
 * @brief Número máximo de argumentos por comando
 */
#define CLI_MAX_ARGS 8

/**
 * @brief Numero maximo de comandos registrados
 */
#define CLI_MAX_COMMANDS 32

/**
 * @brief Tamanho maximo da linha de comando
 */
#define CLI_MAX_CMDLINE_LENGTH 256

/**
 * @brief Tamanho maximo do prompt
 */
#define CLI_PROMPT_MAX_LEN 32

/**
 * @brief Tamanho do historico de comandos
 */
#define CLI_HISTORY_SIZE 100

/* ========================================================================== */
/*                           TIPOS E ESTRUTURAS                               */
/* ========================================================================== */

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
    const char *short_opt;      /**< Opcao curta (ex: "t" para -t), NULL se nao tiver */
    const char *long_opt;       /**< Opcao longa (ex: "timeout" para --timeout), NULL se nao tiver */
    const char *datatype;       /**< Tipo exibido no help (ex: "<ms>", "<ssid>") */
    const char *description;    /**< Descricao do argumento para o help */
    cli_arg_type_t type;        /**< Tipo do argumento */
    bool required;              /**< true = obrigatorio, false = opcional */
} cli_arg_t;

/**
 * @brief Valores dos argumentos parseados (passados ao callback)
 */
typedef struct {
    int int_value;              /**< Valor inteiro (valido se type == CLI_ARG_TYPE_INT) */
    const char *str_value;      /**< Valor string (valido se type == CLI_ARG_TYPE_STRING) */
    bool flag_value;            /**< Valor booleano (valido se type == CLI_ARG_TYPE_FLAG) */
    int count;                  /**< Quantidade de vezes que o argumento apareceu (0 = nao informado) */
} cli_arg_value_t;

/**
 * @brief Contexto passado ao callback do comando
 */
typedef struct {
    int argc;                               /**< Numero de argumentos originais */
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
 */
typedef struct {
    const char *name;               /**< Nome do comando (ex: "join", "version") */
    const char *description;        /**< Descricao exibida no help */
    const char *hint;               /**< Hint opcional mostrado durante digitacao */
    cli_callback_t callback;        /**< Funcao callback executada quando comando e chamado */
    cli_arg_t args[CLI_MAX_ARGS];   /**< Array de argumentos do comando */
    uint8_t arg_count;              /**< Quantidade de argumentos em args[] */
} cli_command_t;

/**
 * @brief Configuracao do console CLI
 */
typedef struct {
    const char *prompt;             /**< Prompt do console (ex: "esp32>"). NULL usa padrao */
    const char *banner;             /**< Mensagem de boas-vindas. NULL usa padrao */
    bool register_help;             /**< true = registra comando 'help' automaticamente */
    bool store_history;             /**< true = salva historico em filesystem (requer particao "storage") */
} cli_config_t;

/** @brief Configuracao padrao do console */
#define CLI_CONFIG_DEFAULT() { \
    .prompt = NULL,            \
    .banner = NULL,            \
    .register_help = true,     \
    .store_history = false,    \
}

/* ========================================================================== */
/*                       FUNCOES DE INICIALIZACAO                             */
/* ========================================================================== */

/**
 * @brief Inicializa o console CLI completo
 *
 * Esta funcao inicializa o periferico de console (UART/USB),
 * a biblioteca linenoise e o esp_console.
 *
 * @param config Configuracao do console (pode ser NULL para usar padrao)
 * @return esp_err_t ESP_OK em sucesso
 */
esp_err_t cli_init(const cli_config_t *config);

/**
 * @brief Inicia o loop principal do console
 *
 * Esta funcao bloqueia e processa comandos continuamente.
 * Retorna apenas em caso de erro ou Ctrl+C.
 *
 * @return esp_err_t ESP_OK se terminou normalmente
 */
esp_err_t cli_run(void);

/**
 * @brief Finaliza o console CLI
 *
 * Libera recursos alocados pelo console.
 */
void cli_deinit(void);

/**
 * @brief Retorna o prompt atual
 *
 * @return const char* Ponteiro para o prompt
 */
const char *cli_get_prompt(void);

/* ========================================================================== */
/*                       FUNCOES DE REGISTRO DE COMANDOS                      */
/* ========================================================================== */

/**
 * @brief Registra um comando no console
 *
 * @param cmd Ponteiro para struct descrevendo o comando
 * @return esp_err_t
 *         - ESP_OK: Sucesso
 *         - ESP_ERR_INVALID_ARG: Parametro invalido
 *         - ESP_ERR_NO_MEM: Sem memoria para alocar argtable
 */
esp_err_t cli_register_command(const cli_command_t *cmd);

/**
 * @brief Registra um comando simples (sem argumentos)
 *
 * Atalho para comandos que nao precisam de argumentos.
 *
 * @param name Nome do comando
 * @param description Descricao para o help
 * @param callback Funcao callback (recebe argc/argv padrao)
 * @return esp_err_t ESP_OK em sucesso
 */
esp_err_t cli_register_simple_command(const char *name,
                                       const char *description,
                                       int (*callback)(int argc, char **argv));

/**
 * @brief Registra multiplos comandos de uma vez
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
