#ifndef TASK_TRATAMENTO_FALHAS_H
#define TASK_TRATAMENTO_FALHAS_H

#include "gerenciador_dados.h"
#include <vector>

class SimulacaoMina;

/**
 * @brief Esta é a nossa única Produtora para o buffer circular.
 *
 * É o "olho" do caminhão, responsável por ler os dados dos sensores
 * (neste caso, simulando-os) e colocá-los no buffer para processamento.
 *
 * @param gerenciadorDados Referência para o objeto GerenciadorDados.
 * @param simulacao Referência para a simulação física.
 * @param eventos Referência para o objeto EventosSistema.
 */
void task_tratamento_falhas(GerenciadorDados& gerenciadorDados, SimulacaoMina& simulacao, EventosSistema& eventos);

#endif // TASK_TRATAMENTO_FALHAS_H