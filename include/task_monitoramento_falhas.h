/**
 * @file task_monitoramento_falhas.h
 * @brief Definição da tarefa de monitoramento e tratamento de falhas.
 */

#ifndef TASK_TRATAMENTO_FALHAS_H
#define TASK_TRATAMENTO_FALHAS_H

#include "gerenciador_dados.h"
#include "eventos_sistema.h"
#include <vector>

class SimulacaoMina;

/**
 * @brief Tarefa responsável por monitorar a saúde do sistema e tratar falhas.
 *
 * Verifica periodicamente os dados dos sensores e o estado da simulação em busca
 * de anomalias (ex: superaquecimento, falhas elétricas). Caso detecte um problema,
 * sinaliza o evento correspondente para que o sistema possa reagir (ex: parada de emergência).
 *
 * @param gerenciadorDados Referência para o gerenciador de dados.
 * @param simulacao Referência para a simulação física (para injeção/verificação de falhas reais).
 * @param eventos Referência para o sistema de sinalização de eventos.
 */
void task_tratamento_falhas(GerenciadorDados& gerenciadorDados, SimulacaoMina& simulacao, EventosSistema& eventos);

#endif // TASK_TRATAMENTO_FALHAS_H