/**
 * @file task_controle_navegacao.h
 * @brief Definição da tarefa de controle de navegação autônoma.
 */

#ifndef TASK_CONTROLE_NAVEGACAO_H
#define TASK_CONTROLE_NAVEGACAO_H

#include "gerenciador_dados.h"

/**
 * @brief Tarefa responsável pelo controle de navegação do veículo.
 *
 * Esta função implementa a lógica de controle de alto nível. Ela consome dados
 * processados do buffer, avalia o estado atual e os objetivos, e gera comandos
 * de atuação (aceleração e direção) para manter o veículo na trajetória desejada
 * ou reagir a obstáculos.
 *
 * @param gerenciadorDados Referência para o objeto compartilhado GerenciadorDados.
 */
void task_controle_navegacao(GerenciadorDados& gerenciadorDados);

#endif // TASK_CONTROLE_NAVEGACAO_H