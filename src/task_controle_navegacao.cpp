#include "task_controle_navegacao.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"
#include "utils/sleep_asynch.h" // Incluindo o utilitário de tempo preciso
#include <boost/asio.hpp>       // Motor assíncrono local
#include <iostream>
#include <functional>           // Para std::function
#include <algorithm> 

struct ControladorEstado {
    float setpoint_velocidade = 8000.0f; 
    float setpoint_angulo = 0.0f;
    
    float kp_vel = 2.0f; 
    float kp_ang = 1.5f; 
};

void task_controle_navegacao(GerenciadorDados& dados, EventosSistema& eventos) {
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

        // VERIFICAÇÃO DE SEGURANÇA VIA EVENTOS (Linha Vermelha)
        if (eventos.verificar_estado_falha()) {
            saida_aceleracao = 0;
            // Se houver falha crítica, paramos imediatamente.
        
        } else if (estado.e_defeito) {
            saida_aceleracao = 0;
            // Mantido para compatibilidade com flags antigas
        
        } else if (!estado.e_automatico) {
            // --- MODO MANUAL ---
            if (comandos.c_acelerar) saida_aceleracao = 50; 
            else saida_aceleracao = 0;

            if (comandos.c_direita) saida_direcao = 45;
            else if (comandos.c_esquerda) saida_direcao = -45;
            else saida_direcao = 0; 

            // Bumpless Transfer: Atualiza setpoints para a realidade atual
            controlador.setpoint_velocidade = static_cast<float>(leituraAtual.i_velocidade);
            controlador.setpoint_angulo = static_cast<float>(leituraAtual.i_angulo_x);
            
        } else {
            // --- MODO AUTOMÁTICO (Controle P + Obstacle Avoidance) ---
            
            // 1. Controle de Velocidade (Mantém velocidade constante)
            float erro_vel = controlador.setpoint_velocidade - static_cast<float>(leituraAtual.i_velocidade);
            saida_aceleracao = (int)(erro_vel * controlador.kp_vel);

            // 2. Navegação Reativa (Wall Following / Center Keeping)
            // Como não temos LIDAR real, vamos usar a posição absoluta para simular
            // um sensor que detecta o centro do corredor.
            
            const float CELL_SIZE = 10.0f;
            float pos_x = static_cast<float>(leituraAtual.i_posicao_x);
            float pos_y = static_cast<float>(leituraAtual.i_posicao_y);
            
            // Calcula a posição relativa dentro da célula (0 a 10)
            float cell_offset_x = fmod(pos_x, CELL_SIZE);
            float cell_offset_y = fmod(pos_y, CELL_SIZE);
            
            // O centro da célula é 5.0
            float erro_centro_x = 5.0f - cell_offset_x;
            float erro_centro_y = 5.0f - cell_offset_y;
            
            // Determina a orientação principal do corredor (N/S ou E/W)
            // Simplificação: Se o ângulo é próximo de 0 ou 180, estamos indo Leste/Oeste -> Corrigir Y
            // Se o ângulo é próximo de 90 ou -90, estamos indo Norte/Sul -> Corrigir X
            
            float angulo_atual = static_cast<float>(leituraAtual.i_angulo_x);
            float target_angle = controlador.setpoint_angulo; // Mantém o objetivo original por padrão

            // Lógica simples de "Manter no Centro"
            if ((angulo_atual >= -45 && angulo_atual <= 45) || (angulo_atual >= 135 || angulo_atual <= -135)) {
                // Movimento Horizontal (Leste/Oeste): Corrigir erro em Y
                // Se erro_centro_y > 0, estamos muito "acima" (y pequeno), precisamos descer (aumentar y)
                // Para aumentar Y indo para Leste (0 graus), precisamos virar levemente para 90 (Esquerda/Direita?)
                // Angulo 0 = Leste. Y aumenta para "Baixo" no grid visual, mas na física Y aumenta para "Sul"?
                // Vamos assumir Y aumenta para baixo.
                // Se Y está pequeno (offset < 5), erro > 0. Precisamos aumentar Y.
                // Se estamos indo para Leste (0), virar para Direita (positivo) aumenta Y?
                // Depende do sistema de coordenadas. Geralmente Y aumenta para baixo em grids.
                // Se Y aumenta para baixo:
                // Indo Leste (0): Virar Direita (>0) aumenta Y.
                // Erro > 0 (estamos acima) -> Queremos descer -> Virar Direita.
                
                // Fator de correção de centralização
                float correcao_centro = erro_centro_y * 5.0f; // Ganho proporcional
                
                // Se estamos indo para Oeste (180/-180):
                // Virar Direita (diminui angulo?) -> Y diminui?
                // Vamos simplificar: Ajustar o setpoint de ângulo para apontar para o centro Y da célula.
                
                // Mas espere, o setpoint_angulo original é fixo em 0.0 no struct?
                // O código original não atualizava o setpoint_angulo no modo automático!
                // Ele só usava o valor inicial 0.0. Isso explica porque ele batia na parede se o corredor virasse.
                // Precisamos de um Planejador de Caminho ou seguir o corredor.
                
                // Como não temos mapa aqui, vamos fazer um "Braitenberg Vehicle" simples:
                // Se bater, vira (já feito na física).
                // Para navegar, vamos tentar manter o ângulo atual se estiver livre, 
                // mas corrigir para o centro da célula.
                
                // Melhoria: O "setpoint_angulo" deve vir de algum lugar.
                // Se não temos path planning, o caminhão vai andar em linha reta (0 graus) para sempre.
                // Vamos fazer ele andar aleatoriamente nas interseções?
                // Ou apenas seguir em frente e corrigir o centro.
                
                // Correção Y:
                if (abs(angulo_atual) < 90) { // Indo Leste
                     target_angle = 0.0f + correcao_centro; 
                } else { // Indo Oeste
                     target_angle = 180.0f - correcao_centro;
                }

            } else {
                // Movimento Vertical (Norte/Sul): Corrigir erro em X
                float correcao_centro = -erro_centro_x * 5.0f; // Invertido?
                // Se X está pequeno (offset < 5), erro > 0. Precisamos aumentar X.
                // Indo Sul (90): Virar Esquerda (<90) aumenta X?
                // 0=Leste, 90=Sul.
                // Virar para 0 (Esquerda relativa ao Sul) aumenta X.
                // Então diminuir ângulo aumenta X.
                // Erro > 0 -> Queremos aumentar X -> Diminuir ângulo.
                
                if (angulo_atual > 0) { // Indo Sul (90)
                    target_angle = 90.0f - correcao_centro;
                } else { // Indo Norte (-90)
                    target_angle = -90.0f + correcao_centro;
                }
            }
            
            controlador.setpoint_angulo = target_angle;

            // Controle de Direção (PID)
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

        // ENVIO DO COMANDO (ESCRITA DIRETA NA MEMÓRIA)
        ComandosAtuador cmd;
        cmd.aceleracao = saida_aceleracao;
        cmd.direcao = saida_direcao;
        dados.setComandosAtuador(cmd);

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