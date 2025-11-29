#include "task_logica_comando.h"
#include "interfaces/i_veiculo_driver.h"
#include "utils/sleep_asynch.h" // Inclua seu utilitário
#include <boost/asio.hpp>       // Necessário para o motor de tempo
#include <iostream>
#include <functional>           // Para std::function

void task_logica_comando(GerenciadorDados& gerenciadorDados, EventosSistema& eventos, IVeiculoDriver& driver) {
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

        // Monitoramento de Temperatura
        if (dados.i_temperatura > 120) {
            estado.e_defeito = true;
            std::cout << "[Logica] DEFEITO: Temperatura critica (" << dados.i_temperatura << ")!" << std::endl;
        }

        // Modos de Operação
        if (comandos.c_automatico) {
            estado.e_automatico = true;
        } else if (comandos.c_man) {
            estado.e_automatico = false;
        }

        // Rearme
        if (comandos.c_rearme && estado.e_defeito) {
            std::cout << "[Logica] REARME recebido. Sistema reiniciado.\n";
            estado.e_defeito = false;
        }

        gerenciadorDados.atualizarEstadoVeiculo(estado);
        
        // --- FIM DA LÓGICA ---

        // 3. Agendamento Preciso (Heartbeat)
        // Isso garante 10Hz cravados, compensando o tempo gasto na lógica acima.
        timer.wait_next_tick(std::chrono::milliseconds(100), 
            [&](const boost::system::error_code& ec) {
                if (!ec) loop_logica(); // Recursão assíncrona
            }
        );
    };

    // 4. Start no Loop
    loop_logica();

    // 5. Bloqueia a thread aqui rodando o loop de eventos
    // Substitui o while(true). Só sai se chamarmos io.stop() ou acabar o trabalho.
    io.run();
}