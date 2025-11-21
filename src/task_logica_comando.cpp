#include "task_logica_comando.h"
#include <iostream>
#include <chrono>
#include <thread>

void task_logica_comando(GerenciadorDados& gerenciadorDados, EventosSistema& eventos) {
    while (true) {
        DadosSensores dados = gerenciadorDados.getDados();
        ComandosOperador comandos = gerenciadorDados.getComandosOperador();
        EstadoVeiculo estado = gerenciadorDados.getEstadoVeiculo();

        if (eventos.verificar_estado_falha()) {
            estado.e_defeito = true;
            std::cout << "[Logica] Falha crítica detectada! Código de falha: " << eventos.get_codigo_falha() << "\n";
            // adicionar lógica para lidar com falha
        }
        // Exemplo: Mudar modo de operação
        if (comandos.c_automatico) {
            estado.e_automatico = true;
        } else if (comandos.c_man) {
            estado.e_automatico = false;
        }

        // Rearme do sistema (limpa defeito)
        if (comandos.c_rearme && estado.e_defeito) {
            std::cout << "[Logica] Comando de REARME recebido. Limpando estado de defeito.\n";
            estado.e_defeito = false;
        }

        gerenciadorDados.atualizarEstadoVeiculo(estado);

        // Aguarda um pouco antes da próxima iteração
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
