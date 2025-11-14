#ifndef TASK_LOGICA_COMANDO_H
#define TASK_LOGICA_COMANDO_H

#include "gerenciador_dados.h"

/**
 * @brief Esta é a task "cérebro supervisor".
 *
 * É um Consumidor de dados e Produtor de estado. Ele decide o que o caminhão
 * pode ou não pode fazer com base nos dados dos sensores, comandos do operador
 * e o estado atual do veículo.
 *
 * @param gerenciadorDados Referência para o objeto GerenciadorDados.
 */
void task_logica_comando(GerenciadorDados& gerenciadorDados);

#endif // TASK_LOGICA_COMANDO_H
