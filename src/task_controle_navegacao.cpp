#include "task_controle_navegacao.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"
#include "utils/sleep_asynch.h" // Incluindo o utilitário de tempo preciso
#include <algorithm>
#include <boost/asio.hpp> // Motor assíncrono local
#include <functional>     // Para std::function
#include <iostream>

struct ControladorEstado {
  // Speed Control (IP)
  float kp_vel = 20.0f;
  float ki_vel = 20.0f;
  float integral_vel = 0.0f;

  // Heading Control (IP)
  float kp_ang = 1.0f;
  float ki_ang = 0.5f;
  float integral_ang = 0.0f;

  // State Feedback Setpoints (Internal)
  float setpoint_velocidade = 0.0f;
  float setpoint_angulo = 0.0f;
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
    float dt = 0.1f; // 10Hz

    // Ler Snapshot (Estado mais recente sem consumir do buffer de log)
    DadosSensores leituraAtual = dados.lerUltimoEstado();
    EstadoVeiculo estado = dados.getEstadoVeiculo();
    ComandosOperador comandos = dados.getComandosOperador();

    int saida_aceleracao = 0;
    int saida_direcao = 0;

    // NEW: Read the Planner's order (Moved up for scope)
    ObjetivoNavegacao objetivo = dados.getObjetivo();

    // VERIFICAÇÃO DE SEGURANÇA VIA EVENTOS (Linha Vermelha)
    if (eventos.verificar_estado_falha()) {
      saida_aceleracao = -100;      // Emergency Brake
      controlador.integral_vel = 0; // Reset integrators on fault
      controlador.integral_ang = 0;

    } else if (estado.e_defeito) {
      saida_aceleracao = 0;
      controlador.integral_vel = 0;
      controlador.integral_ang = 0;

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

      // Bumpless Transfer: Sync setpoints to current state
      // This ensures that when switching to Auto, the error starts at zero.
      controlador.setpoint_velocidade = (float)leituraAtual.i_velocidade;
      controlador.setpoint_angulo = (float)leituraAtual.i_angulo_x;

      // Also reset integrators to avoid windup from previous auto sessions
      controlador.integral_vel = 0;
      controlador.integral_ang = 0;

    } else {

      if (estado.e_automatico && objetivo.ativo) {
        // --- AUTOMATIC MODE (State Space Control - IP Topology) ---

        // 1. Calculate Heading Error FIRST (needed for cornering speed)
        float theta_atual = (float)leituraAtual.i_angulo_x;
        float x_atual = (float)leituraAtual.i_posicao_x;
        float y_atual = (float)leituraAtual.i_posicao_y;

        float dx = objetivo.x_alvo - x_atual;
        float dy = objetivo.y_alvo - y_atual;
        float theta_ref = std::atan2(dy, dx) * 180.0f / M_PI;

        float erro_ang = theta_ref - theta_atual;
        // Normalize error to [-180, 180]
        while (erro_ang > 180)
          erro_ang -= 360;
        while (erro_ang < -180)
          erro_ang += 360;

        // 2. Speed Control (IP) with Cornering Slowdown
        float v_atual = (float)leituraAtual.i_velocidade;
        float v_ref = objetivo.velocidade_alvo;

        // Cornering Logic: Slow down if error is large
        // If error > 10 degrees, reduce speed.
        float erro_abs = std::abs(erro_ang);
        if (erro_abs > 10.0f) {
          // User requested 4 m/s at 90 degrees (Base 20 m/s).
          // Factor = 4/20 = 0.2.
          // 0.2 = 1.0 - (90 / X) -> X = 112.5.
          float factor = 1.0f - (std::min(erro_abs, 112.5f) / 112.5f);

          v_ref = v_ref * factor;
          // Min speed 2.0f to allow slowing down to 4.0f
          if (v_ref < 2.0f)
            v_ref = 2.0f;
        }

        float erro_vel = v_ref - v_atual;
        controlador.integral_vel += erro_vel * dt;

        // Anti-windup (Speed)
        if (controlador.integral_vel > 100.0f)
          controlador.integral_vel = 100.0f;
        if (controlador.integral_vel < -100.0f)
          controlador.integral_vel = -100.0f;

        // IP Control Law: u = -Kp*y + Ki*Integral(e)
        float u_acc = -controlador.kp_vel * v_atual +
                      controlador.ki_vel * controlador.integral_vel;
        saida_aceleracao = (int)u_acc;

        // 3. Heading Control (Pure Pursuit)
        // Replaces PID with a geometric path tracking algorithm.

        const float WHEELBASE = 6.0f; // Truck Length/Wheelbase
        const float K_LOOKAHEAD =
            1.1f; // Lookahead gain (seconds) - User requested 1.1
        const float MIN_LOOKAHEAD =
            2.8f; // Minimum lookahead distance (meters) - User requested 2.8

        // Calculate dynamic Lookahead Distance (Ld)
        float ld = std::max(MIN_LOOKAHEAD, v_atual * K_LOOKAHEAD);

        // Calculate distance to target waypoint
        float dist_to_wp = std::sqrt(dx * dx + dy * dy);

        // Determine Lookahead Point
        float target_x, target_y;
        if (dist_to_wp > ld) {
          float ratio = ld / dist_to_wp;
          target_x = x_atual + dx * ratio;
          target_y = y_atual + dy * ratio;
        } else {
          // If waypoint is closer, aim at the waypoint.
          target_x = objetivo.x_alvo;
          target_y = objetivo.y_alvo;
          // CRITICAL FIX: Do NOT reduce 'ld' to 'dist_to_wp'.
          // Keeping 'ld' large acts as a dampener when close to the target,
          // preventing infinite steering gain and oscillation.
        }

        // Calculate Alpha (Angle between truck heading and lookahead point)
        float dx_p = target_x - x_atual;
        float dy_p = target_y - y_atual;
        float angle_to_p = std::atan2(dy_p, dx_p) * 180.0f / M_PI;

        float alpha = angle_to_p - theta_atual;
        while (alpha > 180)
          alpha -= 360;
        while (alpha < -180)
          alpha += 360;

        // Pure Pursuit Control Law
        // steering_angle = atan(2 * L * sin(alpha) / Ld)
        // Note: alpha must be in radians for sin(), result is radians.
        float alpha_rad = alpha * M_PI / 180.0f;
        float steering_rad =
            std::atan((2.0f * WHEELBASE * std::sin(alpha_rad)) / ld);
        float steering_deg = steering_rad * 180.0f / M_PI;

        // Apply correction to current heading
        // Command = Current Heading + Steering Angle
        float cmd_direcao = theta_atual + steering_deg;

        // Normalize Command to 0-360
        while (cmd_direcao > 360)
          cmd_direcao -= 360;
        while (cmd_direcao < 0)
          cmd_direcao += 360;

        saida_direcao = (int)cmd_direcao;

        // Saturação (Clamp Outputs)
        if (saida_aceleracao > 100)
          saida_aceleracao = 100;
        if (saida_aceleracao < -100)
          saida_aceleracao = -100;
        // Direction is 0-360, no clamp needed other than normalization above.

      } else if (estado.e_automatico && !objetivo.ativo) {
        // Auto mode but no mission? Stop safely.
        saida_aceleracao = -100; // Brake
        // Keep current heading to avoid spinning while braking
        saida_direcao = (int)leituraAtual.i_angulo_x;

        controlador.integral_vel = 0;
        controlador.integral_ang = 0;
      }
    }

    // ENVIO DO COMANDO (ESCRITA DIRETA NA MEMÓRIA)
    ComandosAtuador cmd;
    cmd.aceleracao = saida_aceleracao;
    cmd.direcao = saida_direcao;
    dados.setComandosAtuador(cmd);

    // DEBUG: Print controller state every 1s (approx 10 cycles)
    static int debug_counter = 0;
    if (++debug_counter >= 10) {
      debug_counter = 0;
      // std::cout << "[NAV-DEBUG] Auto:" << estado.e_automatico
      //           << " Obj:" << objetivo.ativo
      //           << " V_Ref:" << objetivo.velocidade_alvo
      //           << " V_Act:" << leituraAtual.i_velocidade
      //           << " Int_Vel:" << controlador.integral_vel
      //           << " U_Acc:" << saida_aceleracao
      //           << " Fault:" << eventos.verificar_estado_falha() <<
      //           std::endl;
    }

    // --- FIM DA LÓGICA ---

    // 3. Agendamento Preciso (Heartbeat de 10Hz)
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