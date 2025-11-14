#include "task_tratamento_sensores.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

void task_tratamento_sensores(GerenciadorDados& gerenciadorDados) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> temp_dist(80, 130);
    std::uniform_int_distribution<int> pos_dist(0, 1000);
    std::uniform_int_distribution<int> ang_dist(0, 360);

    while (true) {
        DadosSensores novosDados;
        novosDados.i_temperatura = temp_dist(generator);
        novosDados.i_posicao_x = pos_dist(generator);
        novosDados.i_posicao_y = pos_dist(generator);
        novosDados.i_angulo_x = ang_dist(generator);
        novosDados.i_falha_eletrica = false; // Simulação simples
        novosDados.i_falha_hidraulica = false; // Simulação simples

        gerenciadorDados.setDados(novosDados);
        std::cout << "Novos dados de sensores gerados." << std::endl;

        // Aguarda um pouco antes de gerar novos dados
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
