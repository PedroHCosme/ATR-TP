#ifndef TASK_TRATAMENTO_SENSORES_H
#define TASK_TRATAMENTO_SENSORES_H

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
 * @param id_caminhao ID do caminhão a ser monitorado.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados, SimulacaoMina& simulacao, int id_caminhao);

/**
 * @brief Calcula o próximo valor da Média Móvel Exponencial (EMA).
 * 
 * Esta função aplica a fórmula da média móvel exponencial, que dá mais peso 
 * aos dados recentes, tornando-a mais sensível a mudanças novas do que a média simples.
 * 
 * @param valor_atual O valor bruto lido do sensor agora.
 * @param media_anterior O valor da EMA calculado no passo anterior.
 * @return float O novo valor da EMA.
 */
float calcular_media_movel_exponencial(float valor_atual, float media_anterior);



#endif // TASK_TRATAMENTO_SENSORES_H
