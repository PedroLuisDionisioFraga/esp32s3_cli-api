/**
 * @file cli-api.c
 * @brief Implementação da API simplificada para comandos de console ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cli-api.h"
#include "esp_console.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"

static const char *TAG = "cli-api";

/**
 * @brief Estrutura interna para armazenar dados do comando registrado
 */
typedef struct {
    const cli_command_t *cmd_def;   /**< Definição original do comando */
    void *argtable[CLI_MAX_ARGS + 1]; /**< Ponteiros para argtable3 (+1 para arg_end) */
    uint8_t arg_count;              /**< Quantidade de argumentos */
} cli_registered_cmd_t;

/** @brief Array de comandos registrados */
static cli_registered_cmd_t s_registered_cmds[CLI_MAX_COMMANDS];

/** @brief Contador de comandos registrados */
static uint8_t s_cmd_count = 0;

/**
 * @brief Wrapper callback que faz parsing dos argumentos e chama o callback do usuário
 */
static int cli_command_wrapper(int argc, char **argv)
{
    /* Encontra o comando registrado pelo nome (argv[0]) */
    cli_registered_cmd_t *reg_cmd = NULL;
    for (int i = 0; i < s_cmd_count; i++) {
        if (strcmp(s_registered_cmds[i].cmd_def->name, argv[0]) == 0) {
            reg_cmd = &s_registered_cmds[i];
            break;
        }
    }

    if (reg_cmd == NULL) {
        ESP_LOGE(TAG, "Comando '%s' não encontrado internamente", argv[0]);
        return 1;
    }

    const cli_command_t *cmd = reg_cmd->cmd_def;

    /* Faz parsing dos argumentos usando argtable3 */
    int nerrors = arg_parse(argc, argv, reg_cmd->argtable);

    /* Se houver erros de parsing, mostra e retorna */
    if (nerrors != 0) {
        struct arg_end *end = reg_cmd->argtable[cmd->arg_count];
        arg_print_errors(stderr, end, argv[0]);
        return 1;
    }

    /* Prepara o contexto para o callback do usuário */
    cli_context_t ctx = {
        .argc = argc,
        .argv = argv,
        .arg_count = cmd->arg_count,
    };

    /* Extrai valores dos argumentos parseados */
    for (int i = 0; i < cmd->arg_count; i++) {
        void *arg = reg_cmd->argtable[i];

        switch (cmd->args[i].type) {
            case CLI_ARG_TYPE_INT: {
                struct arg_int *a = (struct arg_int *)arg;
                ctx.args[i].count = a->count;
                ctx.args[i].int_value = (a->count > 0) ? a->ival[0] : 0;
                break;
            }
            case CLI_ARG_TYPE_STRING: {
                struct arg_str *a = (struct arg_str *)arg;
                ctx.args[i].count = a->count;
                ctx.args[i].str_value = (a->count > 0) ? a->sval[0] : NULL;
                break;
            }
            case CLI_ARG_TYPE_FLAG: {
                struct arg_lit *a = (struct arg_lit *)arg;
                ctx.args[i].count = a->count;
                ctx.args[i].flag_value = (a->count > 0);
                break;
            }
        }
    }

    /* Chama o callback do usuário */
    return cmd->callback(&ctx);
}

esp_err_t cli_register_command(const cli_command_t *cmd)
{
    if (cmd == NULL || cmd->name == NULL || cmd->callback == NULL) {
        ESP_LOGE(TAG, "Parâmetros inválidos");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_cmd_count >= CLI_MAX_COMMANDS) {
        ESP_LOGE(TAG, "Limite de comandos atingido (%d)", CLI_MAX_COMMANDS);
        return ESP_ERR_NO_MEM;
    }

    /* Se não tem argumentos, usa registro simples */
    if (cmd->arg_count == 0) {
        /* Cria wrapper simples que converte para cli_context_t */
        /* Para comandos sem argumentos, registra diretamente */
        const esp_console_cmd_t esp_cmd = {
            .command = cmd->name,
            .help = cmd->description,
            .hint = cmd->hint,
            .func = (int (*)(int, char**))cli_command_wrapper,
        };

        /* Salva referência */
        s_registered_cmds[s_cmd_count].cmd_def = cmd;
        s_registered_cmds[s_cmd_count].arg_count = 0;
        s_cmd_count++;

        return esp_console_cmd_register(&esp_cmd);
    }

    /* Aloca estruturas argtable3 para cada argumento */
    cli_registered_cmd_t *reg_cmd = &s_registered_cmds[s_cmd_count];
    reg_cmd->cmd_def = cmd;
    reg_cmd->arg_count = cmd->arg_count;

    for (int i = 0; i < cmd->arg_count; i++) {
        const cli_arg_t *arg = &cmd->args[i];

        switch (arg->type) {
            case CLI_ARG_TYPE_INT:
                if (arg->required) {
                    reg_cmd->argtable[i] = arg_int1(
                        arg->short_opt, arg->long_opt,
                        arg->datatype, arg->description
                    );
                } else {
                    reg_cmd->argtable[i] = arg_int0(
                        arg->short_opt, arg->long_opt,
                        arg->datatype, arg->description
                    );
                }
                break;

            case CLI_ARG_TYPE_STRING:
                if (arg->required) {
                    reg_cmd->argtable[i] = arg_str1(
                        arg->short_opt, arg->long_opt,
                        arg->datatype, arg->description
                    );
                } else {
                    reg_cmd->argtable[i] = arg_str0(
                        arg->short_opt, arg->long_opt,
                        arg->datatype, arg->description
                    );
                }
                break;

            case CLI_ARG_TYPE_FLAG:
                if (arg->required) {
                    reg_cmd->argtable[i] = arg_lit1(
                        arg->short_opt, arg->long_opt,
                        arg->description
                    );
                } else {
                    reg_cmd->argtable[i] = arg_lit0(
                        arg->short_opt, arg->long_opt,
                        arg->description
                    );
                }
                break;

            default:
                ESP_LOGE(TAG, "Tipo de argumento inválido: %d", arg->type);
                return ESP_ERR_INVALID_ARG;
        }

        if (reg_cmd->argtable[i] == NULL) {
            ESP_LOGE(TAG, "Falha ao alocar argumento %d", i);
            /* Libera argumentos já alocados */
            for (int j = 0; j < i; j++) {
                free(reg_cmd->argtable[j]);
            }
            return ESP_ERR_NO_MEM;
        }
    }

    /* Adiciona arg_end no final (obrigatório para argtable3) */
    reg_cmd->argtable[cmd->arg_count] = arg_end(cmd->arg_count + 1);

    /* Registra o comando no esp_console */
    const esp_console_cmd_t esp_cmd = {
        .command = cmd->name,
        .help = cmd->description,
        .hint = cmd->hint,
        .func = cli_command_wrapper,
        .argtable = reg_cmd->argtable,
    };

    esp_err_t ret = esp_console_cmd_register(&esp_cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar comando '%s': %s", cmd->name, esp_err_to_name(ret));
        /* Libera argtable em caso de erro */
        arg_freetable(reg_cmd->argtable, cmd->arg_count + 1);
        return ret;
    }

    s_cmd_count++;
    ESP_LOGI(TAG, "Comando '%s' registrado com %d argumentos", cmd->name, cmd->arg_count);

    return ESP_OK;
}

esp_err_t cli_register_simple_command(const char *name,
                                       const char *description,
                                       int (*callback)(int argc, char **argv))
{
    if (name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_console_cmd_t cmd = {
        .command = name,
        .help = description,
        .hint = NULL,
        .func = callback,
    };

    esp_err_t ret = esp_console_cmd_register(&cmd);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Comando simples '%s' registrado", name);
    }

    return ret;
}

esp_err_t cli_register_commands(const cli_command_t *commands, size_t count)
{
    if (commands == NULL || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; i++) {
        esp_err_t ret = cli_register_command(&commands[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao registrar comando %zu: %s", i, esp_err_to_name(ret));
            return ret;
        }
    }

    return ESP_OK;
}

