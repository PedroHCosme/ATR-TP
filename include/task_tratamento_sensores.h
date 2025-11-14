#ifndef TASK_TRATAMENTO_SENSORES_H
#define TASK_TRATAMENTO_SENSORES_H

#include "gerenciador_dados.h"

/**
 * @brief Esta é a nossa única Produtora para o buffer circular.
 *
 * É o "olho" do caminhão, responsável por ler os dados dos sensores
 * (neste caso, simulando-os) e colocá-los no buffer para processamento.
 *
 * @param gerenciadorDados Referência para o objeto GerenciadorDados.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados);

void moving_average_filter();



#endif // TASK_TRATAMENTO_SENSORES_H
