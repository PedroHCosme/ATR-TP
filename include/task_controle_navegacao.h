/**
 * @file task_controle_navegacao.h
 * @brief Definição da tarefa de controle de navegação autônoma.
 */

#ifndef TASK_CONTROLE_NAVEGACAO_H
#define TASK_CONTROLE_NAVEGACAO_H

#include "gerenciador_dados.h"
#include "eventos_sistema.h"

class IVeiculoDriver;

/**
 * @brief Tarefa responsável pelo controle de navegação do veículo.
 *
 * Esta função implementa a lógica de controle de alto nível. Ela consome dados
 * processados do buffer, avalia o estado atual e os objetivos, e gera comandos
 * de atuação (aceleração e direção) para manter o veículo na trajetória desejada
 * ou reagir a obstáculos.
 *
 * @param dados Referência para o gerenciador de dados.
 * @param eventos Referência para o sistema de eventos (para parada de emergência).
 */
void task_controle_navegacao(GerenciadorDados& dados, EventosSistema& eventos);

#endif // TASK_CONTROLE_NAVEGACAO_H