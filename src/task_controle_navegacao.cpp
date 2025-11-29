#include "task_controle_navegacao.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"
#include "utils/sleep_asynch.h" // Incluindo o utilitário de tempo preciso
#include <boost/asio.hpp>       // Motor assíncrono local
#include <iostream>
#include <functional>           // Para std::function
#include <algorithm> 

struct ControladorEstado {
    float setpoint_velocidade = 20.0f; // Queremos 20 m/s
    float setpoint_angulo = 0.0f;
    
    float kp_vel = 2.0f; 
    float kp_ang = 1.5f; 
};

void task_controle_navegacao(GerenciadorDados& dados, IVeiculoDriver& driver) {
    // 1. Configuração do Motor de Tempo Local (Loop de eventos dedicado)
    boost::asio::io_context io;
    SleepAsynch timer(io);

    // Estado do controlador persiste entre as chamadas do loop
    ControladorEstado controlador;

    // 2. Definição do Loop Recursivo (Substitui o while(true) bloqueante)
    std::function<void()> loop_controle;

    loop_controle = [&]() {
        // --- INÍCIO DA LÓGICA DE CONTROLE (Crítica: deve rodar rápido) ---

        // Ler Snapshot (Estado mais recente sem consumir do buffer de log)
        DadosSensores leituraAtual = dados.lerUltimoEstado();
        EstadoVeiculo estado = dados.getEstadoVeiculo();
        ComandosOperador comandos = dados.getComandosOperador();
        
        int saida_aceleracao = 0;
        int saida_direcao = 0;

        if (estado.e_defeito) {
            saida_aceleracao = 0;
            // Opcional: Logar apenas na mudança de estado para não poluir
            // std::cout << "[NAVEGACAO] Parada de Emergencia!" << std::endl;
        
        } else if (!estado.e_automatico) {
            // --- MODO MANUAL ---
            if (comandos.c_acelerar) saida_aceleracao = 50; 
            else saida_aceleracao = 0;

            if (comandos.c_direita) saida_direcao = 45;
            else if (comandos.c_esquerda) saida_direcao = -45;
            else saida_direcao = 0; 

            // Bumpless Transfer: Atualiza setpoints para a realidade atual
            // Assim, ao ativar o automático, o erro inicial é zero.
            controlador.setpoint_velocidade = static_cast<float>(leituraAtual.i_velocidade);
            controlador.setpoint_angulo = static_cast<float>(leituraAtual.i_angulo_x);
            
        } else {
            // --- MODO AUTOMÁTICO (Controle P) ---
            
            // Controle de Velocidade
            float erro_vel = controlador.setpoint_velocidade - static_cast<float>(leituraAtual.i_velocidade);
            saida_aceleracao = (int)(erro_vel * controlador.kp_vel);

            // Controle de Direção
            float erro_ang = controlador.setpoint_angulo - static_cast<float>(leituraAtual.i_angulo_x);
            
            // Normalização do erro angular (-180 a 180)
            while (erro_ang > 180) erro_ang -= 360;
            while (erro_ang < -180) erro_ang += 360;

            saida_direcao = (int)(erro_ang * controlador.kp_ang);
            
            // Saturação (Clamp)
            if (saida_aceleracao > 100) saida_aceleracao = 100;
            if (saida_aceleracao < -100) saida_aceleracao = -100;
            if (saida_direcao > 180) saida_direcao = 180;
            if (saida_direcao < -180) saida_direcao = -180;
        }

        // ENVIO DO COMANDO
        driver.setAtuadores(saida_aceleracao, saida_direcao);

        // --- FIM DA LÓGICA ---

        // 3. Agendamento Preciso (Heartbeat de 10Hz)
        // O wait_next_tick calcula o tempo de sono baseando-se no início do ciclo anterior.
        // Se a lógica acima demorar 5ms, ele dorme 95ms. Se demorar 20ms, dorme 80ms.
        timer.wait_next_tick(std::chrono::milliseconds(100), 
            [&](const boost::system::error_code& ec) {
                if (!ec) loop_controle(); // Próximo ciclo
            }
        );
    };

    // 4. Pontapé inicial
    loop_controle();

    // 5. Entra no loop de eventos (Bloqueia a thread t_navegacao aqui)
    io.run();
}