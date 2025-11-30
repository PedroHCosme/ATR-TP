#include "task_logica_comando.h"
#include "utils/sleep_asynch.h" // Inclua seu utilitário
#include <boost/asio.hpp>       // Necessário para o motor de tempo
#include <functional>
#include <iostream>

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

    // Monitoramento de Falhas via Eventos (Linha Vermelha)
    // A Lógica de Comando reage ao evento setando o estado oficial do veículo
    if (eventos.verificar_estado_falha()) {
      if (!estado.e_defeito) {
        estado.e_defeito = true;
        // std::cout << "[Logica] Evento de falha detectado! Entrando em modo
        // DEFEITO." << std::endl;
      }
    } else {
      // Se já está em DEFEITO, verifica se houve comando de REARME (Reset)
      // O rearme só é processado se o operador solicitar (via botão/tecla)
      // ComandosOperador cmd = dados.getComandosOperador(); // 'dados' não tem
      // getComandosOperador, usar 'comandos'
      if (comandos.c_rearme) { // Usando 'comandos' que já foi lido
        // Resetar falhas
        eventos.resetar_falhas();

        // Voltar para MANUAL inicialmente por segurança
        estado.e_defeito = false;
        estado.e_automatico = false;

        // std::cout << "[Logica] REARME recebido. Resetando falhas e
        // reiniciando sistema.\n";
      }
    }

    // Modos de Operação
    if (comandos.c_automatico) {
      estado.e_automatico = true;
    } else if (comandos.c_man) {
      estado.e_automatico = false;
    }

    // Rearme (Esta parte foi movida/integrada no bloco 'else' acima para evitar
    // duplicação e garantir lógica correta) A lógica de rearme agora está
    // dentro do 'else' do verificar_estado_falha() para garantir que o rearme
    // só ocorra se não houver falha ativa no momento da verificação. Se o
    // rearme for acionado enquanto uma falha ainda está presente, ele não terá
    // efeito imediato. A linha abaixo é redundante se a lógica de rearme
    // estiver no 'else' acima. if (comandos.c_rearme && estado.e_defeito) {
    //     std::cout << "[Logica] REARME recebido. Resetando falhas e
    //     reiniciando sistema.\n"; estado.e_defeito = false;
    //     eventos.resetar_falhas(); // Limpa a flag no sistema de eventos
    // }

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