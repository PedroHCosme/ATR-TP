#include <iostream>
#include <thread>
#include "gerenciador_dados.h"
#include "task_tratamento_sensores.h"
#include "task_logica_comando.h"

int main() {
    GerenciadorDados gerenciadorDados;

    // Inicializa o estado do veículo e os comandos do operador
    EstadoVeiculo estadoInicial = {false, false}; // Sem defeito, modo manual
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    ComandosOperador comandosIniciais = {false, true, false, false, false, false}; // Modo manual, sem outros comandos
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // Cria e inicia as threads
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados));
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados));

    // Aguarda as threads terminarem
    t1.join();
    t2.join();

    return 0;
}

