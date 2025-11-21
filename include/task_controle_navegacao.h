#ifndef TASK_CONTROLE_NAVEGACAO_H
#define TASK_CONTROLE_NAVEGACAO_H

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
void task_controle_navegacao(GerenciadorDados& gerenciadorDados);

#endif // TASK_CONTROLE_NAVEGACAO_H