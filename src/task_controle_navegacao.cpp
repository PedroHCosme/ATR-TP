#include "task_controle_navegacao.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"
#include "utils/sleep_asynch.h" // Incluindo o utilitário de tempo preciso
#include <algorithm>
#include <boost/asio.hpp> // Motor assíncrono local
#include <functional>     // Para std::function
#include <iostream>

struct ControladorEstado {
  float setpoint_velocidade = 8000.0f;
  float setpoint_angulo = 0.0f;

  float kp_vel = 1.0f;
  float kp_ang = 1.0f;
};

void task_controle_navegacao(GerenciadorDados &dados, EventosSistema &eventos) {
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
      if (comandos.c_acelerar)
        saida_aceleracao = 50;
      else
        saida_aceleracao = 0;

      if (comandos.c_direita)
        saida_direcao = 45;
      else if (comandos.c_esquerda)
        saida_direcao = -45;
      else
        saida_direcao = 0;

      // Bumpless Transfer: Atualiza setpoints para a realidade atual
      controlador.setpoint_velocidade =
          static_cast<float>(leituraAtual.i_velocidade);
      controlador.setpoint_angulo = static_cast<float>(leituraAtual.i_angulo_x);

    } else {

      // NEW: Read the Planner's order
      ObjetivoNavegacao objetivo = dados.getObjetivo();

      if (estado.e_automatico && objetivo.ativo) {
        // --- AUTOMATIC MODE ---

        // 1. Calculate Heading Error based on X,Y Target
        float dx = objetivo.x_alvo - (float)leituraAtual.i_posicao_x;
        float dy = objetivo.y_alvo - (float)leituraAtual.i_posicao_y;

        // Desired angle (Atan2 returns radians, convert to degrees)
        // atan2(y, x) returns angle from X axis (East).
        // Our system: 0=East, 90=South (Wait, let's verify coordinates)
        // If 0=East, 90=North? Or South?
        // Standard math: 0=East, 90=North (Counter-Clockwise).
        // Our truck: "0 = Leste", "90 = Sul" (Clockwise?) -> This is common in
        // screen coords (Y down). Let's assume standard atan2 and check sign.
        // atan2(dy, dx) gives angle in radians.

        float angulo_desejado_rad = std::atan2(dy, dx);
        float angulo_desejado_graus = angulo_desejado_rad * (180.0f / 3.14159f);

        // 2. Feed the PIDs
        // Control Speed
        float erro_vel =
            objetivo.velocidade_alvo - (float)leituraAtual.i_velocidade;
        saida_aceleracao = (int)(erro_vel * controlador.kp_vel);

        // Control Steering
        float erro_ang = angulo_desejado_graus - (float)leituraAtual.i_angulo_x;

        // Normalize error -180 to 180
        while (erro_ang > 180)
          erro_ang -= 360;
        while (erro_ang < -180)
          erro_ang += 360;

        // Deadband to prevent jitter
        if (std::abs(erro_ang) < 2.0f) {
          erro_ang = 0.0f;
        }

        saida_direcao = (int)(erro_ang * controlador.kp_ang);

        // Saturação (Clamp)
        if (saida_aceleracao > 100)
          saida_aceleracao = 100;
        if (saida_aceleracao < -100)
          saida_aceleracao = -100;
        if (saida_direcao > 180)
          saida_direcao = 180;
        if (saida_direcao < -180)
          saida_direcao = -180;

      } else if (estado.e_automatico && !objetivo.ativo) {
        // Auto mode but no mission? Stop.
        saida_aceleracao = 0;
        saida_direcao = 0;
      }
    }

    // ENVIO DO COMANDO (ESCRITA DIRETA NA MEMÓRIA)
    ComandosAtuador cmd;
    cmd.aceleracao = saida_aceleracao;
    cmd.direcao = saida_direcao;
    dados.setComandosAtuador(cmd);

    // --- FIM DA LÓGICA ---

    // 3. Agendamento Preciso (Heartbeat de 10Hz)
    // O wait_next_tick calcula o tempo de sono baseando-se no início do ciclo
    // anterior. Se a lógica acima demorar 5ms, ele dorme 95ms. Se demorar
    // 20ms, dorme 80ms.
    timer.wait_next_tick(std::chrono::milliseconds(100),
                         [&](const boost::system::error_code &ec) {
                           if (!ec)
                             loop_controle(); // Próximo ciclo
                         });
  };

  // 4. Pontapé inicial
  loop_controle();

  // 5. Entra no loop de eventos (Bloqueia a thread t_navegacao aqui)
  io.run();
}