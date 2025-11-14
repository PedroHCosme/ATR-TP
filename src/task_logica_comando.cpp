#include "task_logica_comando.h"
#include <iostream>
#include <chrono>
#include <thread>

void task_logica_comando(GerenciadorDados& gerenciadorDados) {
    while (true) {
        DadosSensores dados = gerenciadorDados.getDados();
        ComandosOperador comandos = gerenciadorDados.getComandosOperador();
        EstadoVeiculo estado = gerenciadorDados.getEstadoVeiculo();

        // Lógica de decisão com base nos dados, comandos e estado
        // Exemplo: verificar temperatura
        if (dados.i_temperatura > 120) {
            estado.e_defeito = true;
            std::cout << "Defeito: Temperatura do motor muito alta!" << std::endl;
        } else if (dados.i_temperatura > 95) {
            std::cout << "Alerta: Temperatura do motor elevada!" << std::endl;
        }

        // Exemplo: Mudar modo de operação
        if (comandos.c_automatico) {
            estado.e_automatico = true;
        } else if (comandos.c_man) {
            estado.e_automatico = false;
        }

        gerenciadorDados.atualizarEstadoVeiculo(estado);

        // Aguarda um pouco antes da próxima iteração
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
