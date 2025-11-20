#include <iostream>
#include <thread>
#include "gerenciador_dados.h"
#include "task_tratamento_sensores.h"
#include "task_logica_comando.h"

/**
 * @brief Função principal do sistema.
 * 
 * Inicializa o gerenciador de dados, configura o estado inicial do veículo
 * e inicia as threads responsáveis pelas tarefas do sistema.
 * 
 * @return int Código de saída do programa.
 */
int main() {
    // Instancia o gerenciador de dados compartilhado
    GerenciadorDados gerenciadorDados;

    // Inicializa o estado do veículo e os comandos do operador
    EstadoVeiculo estadoInicial = {false, false}; // Sem defeito, modo manual
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    ComandosOperador comandosIniciais = {false, true, false, false, false, false}; // Modo manual, sem outros comandos
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // Cria e inicia as threads
    // t1: Produtora de dados (Lê sensores e escreve no buffer)
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados));
    
    // t2: Consumidora de dados (Lê do buffer e toma decisões)
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados));

    // Aguarda as threads terminarem (loop infinito nas tasks, então o main fica bloqueado aqui)
    t1.join();
    t2.join();

    return 0;
}

