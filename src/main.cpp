#include <iostream>
#include <thread>
#include <functional> 
#include <boost/asio.hpp> 

// Includes do Sistema
#include "gerenciador_dados.h"
#include "mine_generator.h"
#include "simulacao_mina.h"
#include "drivers/simulacao_driver.h"
#include "utils/sleep_asynch.h"
#include "eventos_sistema.h"

// Includes das Tasks
#include "task_tratamento_sensores.h"
#include "task_logica_comando.h"
#include "task_controle_navegacao.h" 
// #include "task_monitoramento_falhas.h" // (Implementar depois)

/**
 * @brief Função principal do sistema.
 */
int main() {
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
    
    loop_sim_mina = [&simulacao, &sleepAsynch, &loop_sim_mina]() {
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

    // --- 5. CRIAÇÃO DAS THREADS (O SISTEMA EMBARCADO) ---

    // Thread A: Monitoramento (Apenas para debug visual)
    std::thread t_monitor([&gerenciadorDados]() {
        while(true) {
            int tamanho = gerenciadorDados.getContadorDados();
            EstadoVeiculo est = gerenciadorDados.getEstadoVeiculo();
            std::cout << "[MONITOR] Buffer: " << tamanho << "/200 | "
                      << "Modo: " << (est.e_automatico ? "AUTO" : "MANUAL") 
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // Thread B: Motor do ASIO (Roda a simulação física)
    std::thread t_sim([&io]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io.get_executor());
        io.run();
    });

    // Thread 1: Tratamento de Sensores (PRODUTOR)
    // Recebe o driverUniversal como ISensorDriver
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados), std::ref(driverUniversal), 0);
    
    // Thread 2: Lógica de Comando (CONSUMIDOR / GERENTE)
    // Lê do buffer e atualiza o estado (Auto/Manual/Falha)
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados), std::ref(eventos), std::ref(driverUniversal));

    // Thread 3: Controle de Navegação (CONSUMIDOR / PILOTO)
    // Lê estado e envia comandos para o driverUniversal (como IVeiculoDriver)
    std::thread t_navegacao(task_controle_navegacao, std::ref(gerenciadorDados), std::ref(driverUniversal));

    // Thread 4: Monitoramento de Falhas (Se implementada)
    // std::thread t3(task_monitoramento_falhas, ...);

    // --- 6. AGUARDA O FIM (JOIN) ---
    // Como as tasks têm loops infinitos, o programa fica preso aqui rodando.
    t_sim.join();
    t_monitor.join();
    t1.join();
    t2.join();
    t_navegacao.join();

    return 0;
}