/**
 * @file task_tratamento_sensores.h
 * @brief Definição da tarefa de aquisição e processamento de sensores.
 */

#ifndef TASK_TRATAMENTO_SENSORES_H
#define TASK_TRATAMENTO_SENSORES_H

#include "gerenciador_dados.h"
#include <vector>

class SimulacaoMina;

/**
 * @brief Tarefa produtora responsável pela leitura e pré-processamento dos sensores.
 *
 * Esta função simula a aquisição de dados consultando o estado "real" na simulação física,
 * aplica ruído gaussiano para simular imperfeições dos sensores reais, e aplica
 * filtros (como EMA) antes de disponibilizar os dados no buffer compartilhado.
 *
 * @param gerenciadorDados Referência para o objeto GerenciadorDados onde os dados serão escritos.
 * @param simulacao Referência para a simulação física de onde a "verdade" é lida.
 * @param id_caminhao ID do caminhão cujos sensores estão sendo lidos.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados, SimulacaoMina& simulacao, int id_caminhao);

/**
 * @brief Calcula o próximo valor da Média Móvel Exponencial (EMA).
 * 
 * Filtro digital simples utilizado para suavizar leituras ruidosas dos sensores.
 * A fórmula utilizada é: EMA_atual = (Valor_atual - EMA_anterior) * K + EMA_anterior.
 * 
 * @param valor_atual O valor bruto lido do sensor no instante atual.
 * @param media_anterior O valor da EMA calculado no passo anterior.
 * @return float O novo valor suavizado da EMA.
 */
float calcular_media_movel_exponencial(float valor_atual, float media_anterior);



#endif // TASK_TRATAMENTO_SENSORES_H
