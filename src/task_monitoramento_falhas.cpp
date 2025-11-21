#include "task_monitoramento_falhas.h"
#include "simulacao_mina.h"
#include <iostream>
#include <chrono>
#include <thread>

/**
 * @brief Tarefa responsável pelo monitoramento de falhas.
 * 
 * Monitora continuamente os dados dos sensores, detecta anomalias e envia
 * eventos para a task_logica_comando através do GerenciadorDados.
 * 
 * @param gerenciadorDados Referência para o gerenciador de dados compartilhado.
 * @param simulacao Referência para a simulação física.
 * @param id_caminhao ID do caminhão a ser monitorado.
 */
void task_monitoramento_falhas(GerenciadorDados& gerenciadorDados, SimulacaoMina& simulacao, EventosSistema& eventos) {
    // Flags para evitar spam de eventos repetidos
    bool falha_eletrica_reportada = false;
    bool falha_hidraulica_reportada = false;
    bool temperatura_critica_reportada = false;

    while (true) {
        // Lê os dados mais recentes dos sensores do buffer
        DadosSensores dadosAtuais = gerenciadorDados.getDados();

        if (dadosAtuais.i_temperatura > 120) {
            eventos.sinalizar_falha(1); // 1 = Superaquecimento
        }
        else if (dadosAtuais.i_falha_eletrica) {
            eventos.sinalizar_falha(2); // 2 = Elétrica
        }
        else if (dadosAtuais.i_falha_hidraulica) {
            eventos.sinalizar_falha(3); // 3 = Hidráulica
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}