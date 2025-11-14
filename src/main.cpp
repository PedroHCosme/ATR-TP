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

    return 0;
}

