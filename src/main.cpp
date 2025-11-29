#include <iostream>
#include <thread>
#include <functional> 
#include <boost/asio.hpp> 
#include <fstream> 

// Includes do Sistema
#include "gerenciador_dados.h"
#include "mine_generator.h"
#include "simulacao_mina.h"
#include "drivers/simulacao_driver.h"
#include "utils/sleep_asynch.h"
#include "eventos_sistema.h"
#include "server_ipc.h" // Nova Interface via Socket
#include "interface_caminhao.h" // Nova Interface Local (Cockpit)

// Includes das Tasks
#include "task_tratamento_sensores.h"
#include "task_logica_comando.h"
#include "task_controle_navegacao.h" 
#include "task_monitoramento_falhas.h"

/**
 * @brief Função principal do sistema.
 */
int main() {
    // Redireciona std::cout para arquivo para não quebrar a TUI (ncurses)
    std::ofstream logfile("simulacao.log");
    std::streambuf* cout_backup = std::cout.rdbuf();
    std::cout.rdbuf(logfile.rdbuf());
    
    // Redireciona std::cerr também, pois erros podem quebrar a TUI
    std::streambuf* cerr_backup = std::cerr.rdbuf();
    std::cerr.rdbuf(logfile.rdbuf());

    // --- 1. SETUP INICIAL ---
    boost::asio::io_context io; 
    SleepAsynch sleepAsynch(io); 

    std::cout << "Gerando mapa da mina..." << std::endl;
    MineGenerator mineGen(61, 61); 
    mineGen.generate();
    std::cout << "Mapa gerado com sucesso!" << std::endl;

    // --- 2. INSTANCIAÇÃO DOS OBJETOS COMPARTILHADOS ---
    GerenciadorDados gerenciadorDados;
    SimulacaoMina simulacao(mineGen.getMinefield(), 1); 
    SimulacaoDriver driverUniversal(simulacao);
    EventosSistema eventos;

    // --- 3. CONFIGURAÇÃO DE ESTADO INICIAL ---
    EstadoVeiculo estadoInicial = {false, false}; 
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    ComandosOperador comandosIniciais = {
        false, true, false, false, false, false
    }; 
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // --- 4. LOOP DA SIMULAÇÃO FÍSICA (ASSÍNCRONO) ---
    std::function<void()> loop_sim_mina;
    
    loop_sim_mina = [&simulacao, &sleepAsynch, &loop_sim_mina, &gerenciadorDados]() {
        ComandosAtuador cmd = gerenciadorDados.getComandosAtuador();
        simulacao.setComandoAtuador(0, cmd.aceleracao, cmd.direcao);
        simulacao.atualizar_passo_tempo();
        sleepAsynch.wait_next_tick(std::chrono::milliseconds(100), 
            [&loop_sim_mina](const boost::system::error_code& ec) {
                if (!ec) loop_sim_mina(); 
            }
        );
    };

    loop_sim_mina();

    std::thread t_sim([&io]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io.get_executor());
        io.run();
    });

    // --- 5. CRIAÇÃO DAS THREADS (O SISTEMA EMBARCADO) ---
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados), std::ref(driverUniversal), 0);
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados), std::ref(eventos));
    std::thread t_navegacao(task_controle_navegacao, std::ref(gerenciadorDados), std::ref(eventos));
    std::thread t3(task_monitoramento_falhas, std::ref(gerenciadorDados), std::ref(eventos), std::ref(driverUniversal));

    // --- 6. SERVIDOR IPC (Substitui TUI) ---
    // Roda na thread principal e gerencia a comunicação com o Python
    ServerIPC server(gerenciadorDados, simulacao, eventos, mineGen.getMinefield());
    server.start();

    // --- 7. INTERFACE LOCAL (COCKPIT) ---
    // Roda na thread principal (necessário para ncurses)
    InterfaceCaminhao cockpit(gerenciadorDados, eventos);
    cockpit.init();
    cockpit.run(); // Bloqueia aqui até o usuário sair (Q)
    cockpit.close();

    // --- 8. ENCERRAMENTO ---
    // Restaura buffers (opcional, pois o programa vai sair)
    std::cout.rdbuf(cout_backup);
    std::cerr.rdbuf(cerr_backup);
    
    std::cout << "Encerrando sistema..." << std::endl;
    server.stop();
    io.stop();
    
    t_sim.detach();
    t1.detach();
    t2.detach();
    t3.detach();
    t_navegacao.detach();

    return 0;
}