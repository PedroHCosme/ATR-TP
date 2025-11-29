#include "task_controle_navegacao.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm> // Para std::clamp

struct ControladorEstado {
    float setpoint_velocidade = 20.0f; // Queremos 20 m/s
    float setpoint_angulo = 0.0f;
    
    float kp_vel = 2.0f; 
    float kp_ang = 1.5f; 
};

void task_controle_navegacao(GerenciadorDados& dados, IVeiculoDriver& driver) {
    ControladorEstado controlador;

    while (true) {
        // 1. Ler Snapshot
        DadosSensores leituraAtual = dados.lerUltimoEstado();
        EstadoVeiculo estado = dados.getEstadoVeiculo();
        ComandosOperador comandos = dados.getComandosOperador();
        
        int saida_aceleracao = 0;
        int saida_direcao = 0;

        if (estado.e_defeito) {
            saida_aceleracao = 0;
            std::cout << "[NAVEGACAO] Parada de Emergencia!" << std::endl;
        
        } else if (!estado.e_automatico) {
            // --- MODO MANUAL ---
            if (comandos.c_acelerar) saida_aceleracao = 50; 
            else saida_aceleracao = 0;

            if (comandos.c_direita) saida_direcao = 45;
            else if (comandos.c_esquerda) saida_direcao = -45;
            else saida_direcao = 0; // Se não apertar nada, zera direção

            // Bumpless Transfer: Atualiza o setpoint para a velocidade real atual
            controlador.setpoint_velocidade = static_cast<float>(leituraAtual.i_velocidade);
            controlador.setpoint_angulo = static_cast<float>(leituraAtual.i_angulo_x);
            
        } else {
            // --- MODO AUTOMÁTICO (Controle P) ---
            
            // Controle de Velocidade
            // Agora leituraAtual.i_velocidade existe!
            float erro_vel = controlador.setpoint_velocidade - static_cast<float>(leituraAtual.i_velocidade);
            saida_aceleracao = (int)(erro_vel * controlador.kp_vel);

            // Controle de Direção
            float erro_ang = controlador.setpoint_angulo - static_cast<float>(leituraAtual.i_angulo_x);
            
            // Correção para o erro angular (menor caminho no círculo)
            // Ex: Se erro for 350 graus, melhor virar -10.
            while (erro_ang > 180) erro_ang -= 360;
            while (erro_ang < -180) erro_ang += 360;

            saida_direcao = (int)(erro_ang * controlador.kp_ang);
            
            // Saturação (Clamp)
            if (saida_aceleracao > 100) saida_aceleracao = 100;
            if (saida_aceleracao < -100) saida_aceleracao = -100;
            if (saida_direcao > 180) saida_direcao = 180;
            if (saida_direcao < -180) saida_direcao = -180;
            
            // Debug (Opcional)
            // std::cout << "Auto: Vel=" << leituraAtual.i_velocidade << " Acel=" << saida_aceleracao << std::endl;
        }

        // ENVIO DO COMANDO
        driver.setAtuadores(saida_aceleracao, saida_direcao);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}