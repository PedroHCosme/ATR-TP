#include <iostream>
#include <thread>
#include <functional> 
#include <boost/asio.hpp> 
#include <fstream> // Para ofstream

// Includes do Sistema
#include "gerenciador_dados.h"
#include "mine_generator.h"
#include "simulacao_mina.h"
#include "drivers/simulacao_driver.h"
#include "utils/sleep_asynch.h"
#include "eventos_sistema.h"
#include "interface_simulacao.h" // TUI

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
    // Usamos rdbuf para redirecionar apenas o stream C++, mantendo stdout (C) livre para ncurses
    std::ofstream logfile("simulacao.log");
    std::streambuf* cout_backup = std::cout.rdbuf();
    std::cout.rdbuf(logfile.rdbuf());

    // --- 1. SETUP INICIAL ---
    boost::asio::io_context io; // Motor assíncrono
    SleepAsynch sleepAsynch(io); // Controlador de tempo preciso

    std::cout << "Gerando mapa da mina..." << std::endl;
    MineGenerator mineGen(61, 61); // Mapa maior para navegar
    mineGen.generate();
    // mineGen.print(); // Comentei para não poluir o terminal na execução
    std::cout << "Mapa gerado com sucesso!" << std::endl;

    // --- 2. INSTANCIAÇÃO DOS OBJETOS COMPARTILHADOS ---
    
    // O Monitor (Buffer Circular + Estados)
    GerenciadorDados gerenciadorDados;

    // A Física (Mundo Real Simulado)
    SimulacaoMina simulacao(mineGen.getMinefield(), 1); // 1 caminhão
    
    // O Adaptador (HAL - Hardware Abstraction Layer)
    // Ele conecta a simulação às interfaces ISensorDriver e IVeiculoDriver
    SimulacaoDriver driverUniversal(simulacao);

    // Sistema de Eventos (Sinalização de Falhas)
    EventosSistema eventos;

    // --- 3. CONFIGURAÇÃO DE ESTADO INICIAL ---
    EstadoVeiculo estadoInicial = {false, false}; // Sem defeito, modo manual
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    // Começamos em MANUAL, com FREIO (sem acelerar)
    ComandosOperador comandosIniciais = {
        false, // c_automatico
        true,  // c_man
        false, // c_rearme
        false, // c_acelerar
        false, // c_direita
        false  // c_esquerda
    }; 
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // --- 4. LOOP DA SIMULAÇÃO FÍSICA (ASSÍNCRONO) ---
    // Este loop roda a "física do mundo" independente das tasks do software embarcado
    std::function<void()> loop_sim_mina;
    
    loop_sim_mina = [&simulacao, &sleepAsynch, &loop_sim_mina, &gerenciadorDados]() {
        // 0. Lê comandos de atuação da memória compartilhada (GerenciadorDados)
        // Isso simula o hardware lendo os registradores de atuação escritos pelo controle
        ComandosAtuador cmd = gerenciadorDados.getComandosAtuador();
        simulacao.setComandoAtuador(0, cmd.aceleracao, cmd.direcao);

        // 1. Atualiza a física (movimento, temperatura, colisões)
        simulacao.atualizar_passo_tempo();

        // 2. Agenda a próxima execução para manter 10Hz (100ms) cravados
        sleepAsynch.wait_next_tick(std::chrono::milliseconds(100), 
            [&loop_sim_mina](const boost::system::error_code& ec) {
                if (!ec) loop_sim_mina(); 
            }
        );
    };

    // Start na simulação
    loop_sim_mina();

    // Thread B: Motor do ASIO (Roda a simulação física)
    std::thread t_sim([&io]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io.get_executor());
        io.run();
    });

    // --- 5. CRIAÇÃO DAS THREADS (O SISTEMA EMBARCADO) ---

    // Thread 1: Tratamento de Sensores (PRODUTOR)
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados), std::ref(driverUniversal), 0);
    
    // Thread 2: Lógica de Comando (CONSUMIDOR / GERENTE)
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados), std::ref(eventos));

    // Thread 3: Controle de Navegação (CONSUMIDOR / PILOTO)
    std::thread t_navegacao(task_controle_navegacao, std::ref(gerenciadorDados), std::ref(eventos));

    // Thread 4: Monitoramento de Falhas 
    std::thread t3(task_monitoramento_falhas, std::ref(gerenciadorDados), std::ref(eventos), std::ref(driverUniversal));

    // --- 6. INTERFACE GRÁFICA (TUI) ---
    // Roda na thread principal e bloqueia até o usuário sair
    InterfaceSimulacao interface(simulacao, gerenciadorDados, eventos, mineGen.getMinefield());
    interface.init();
    interface.run(); // Bloqueia aqui
    interface.close();

    // --- 7. ENCERRAMENTO ---
    // Quando a interface fecha, forçamos a saída (pois as threads estão em loop infinito)
    // O ideal seria ter um sinal de shutdown, mas exit(0) resolve para a simulação.
    
    // Restaura cout antes de sair (opcional, mas boa prática)
    std::cout.rdbuf(cout_backup);
    std::cout << "Encerrando simulacao..." << std::endl;
    
    io.stop();
    
    // Detach nas threads para permitir o encerramento do processo
    t_sim.detach();
    t1.detach();
    t2.detach();
    t3.detach();
    t_navegacao.detach();

    return 0;
}