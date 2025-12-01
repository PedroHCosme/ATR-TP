#include "task_logica_comando.h"
#include "utils/sleep_asynch.h" // Inclua seu utilitário
#include <boost/asio.hpp>       // Necessário para o motor de tempo
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

void task_logica_comando(GerenciadorDados &gerenciadorDados,
                         EventosSistema &eventos) {
  // 1. Configuração do Motor de Tempo Local (Exclusivo desta thread)
  boost::asio::io_context io;
  SleepAsynch timer(io);

  // 2. Definição do Loop Recursivo (Substitui o while(true))
  // Usamos std::function para que o lambda possa chamar a si mesmo
  std::function<void()> loop_logica;

  loop_logica = [&]() {
    // --- INÍCIO DA LÓGICA (Cópia da sua lógica original) ---

    DadosSensores dados = gerenciadorDados.lerUltimoEstado();
    ComandosOperador comandos = gerenciadorDados.getComandosOperador();
    EstadoVeiculo estado = gerenciadorDados.getEstadoVeiculo();

    // --- LÓGICA DE REARME (Prioridade Máxima) ---
    if (comandos.c_rearme) {
      bool falha_ativa = eventos.verificar_estado_falha();
      int codigo = eventos.getCodigoFalha();

      // Se houver falha de colisão (Código 4), executa manobra de recuo
      if (falha_ativa && codigo == 4) {
        // std::cout << "[Logica] Rearme de COLISÃO. Iniciando Back-Off..." <<
        // std::endl;

        // 1. Recuar (Ré)
        gerenciadorDados.setComandosAtuador({-50, 0}); // -50% força, 0 direção
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Bloqueia por 2s

        // 2. Parar
        gerenciadorDados.setComandosAtuador({0, 0});

        // std::cout << "[Logica] Back-Off concluído." << std::endl;
      }

      // Resetar falhas (para qualquer tipo de falha)
      eventos.resetar_falhas();

      // Modos de Operação (Prioridade para MANUAL)
      if (comandos.c_man) {
        estado.e_automatico = false;
      } else if (comandos.c_automatico) {
        estado.e_automatico = true;
      }
      estado.e_defeito = false;

      // std::cout << "[Logica] Sistema REARMADO." << std::endl;
    }

    // Monitoramento de Falhas (Se não foi rearmado agora)
    if (eventos.verificar_estado_falha()) {
      if (!estado.e_defeito) {
        estado.e_defeito = true;
      }
    }

    gerenciadorDados.atualizarEstadoVeiculo(estado);

    // --- FIM DA LÓGICA ---

    // 3. Agendamento Preciso (Heartbeat)
    // Isso garante 10Hz cravados, compensando o tempo gasto na lógica acima.
    timer.wait_next_tick(std::chrono::milliseconds(100),
                         [&](const boost::system::error_code &ec) {
                           if (!ec)
                             loop_logica(); // Recursão assíncrona
                         });
  };

  // 4. Start no Loop
  loop_logica();

  // 5. Bloqueia a thread aqui rodando o loop de eventos
  // Substitui o while(true). Só sai se chamarmos io.stop() ou acabar o
  // trabalho.
  io.run();
}