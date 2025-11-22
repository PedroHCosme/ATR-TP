#include <iostream>
#include <thread>
#include <functional> 
#include <boost/asio.hpp> 
#include "gerenciador_dados.h"
#include "task_tratamento_sensores.h"
// #include "task_logica_comando.h"
// #include "task_monitoramento_falhas.h"
#include "mine_generator.h"
#include "simulacao_mina.h"
#include "drivers/simulacao_driver.h"
#include "utils/sleep_asynch.h"

/**
 * @brief Função principal do sistema.
 * 
 * Inicializa o gerenciador de dados, configura o estado inicial do veículo
 * e inicia as threads responsáveis pelas tarefas do sistema.
 * 
 * @return int Código de saída do programa.
 */
int main() {
    boost::asio::io_context io; // Contexto para operações assíncronas
    // Gera e imprime o mapa da mina
    std::cout << "Gerando mapa da mina..." << std::endl;
    MineGenerator mineGen(7, 7); // Tamanho 31x31
    mineGen.generate();
    mineGen.print();
    std::cout << "Mapa gerado com sucesso!" << std::endl;

    //Instancia Sleep assincrono
    SleepAsynch sleepAsynch(io);
    

    // Instancia o gerenciador de dados compartilhado
    GerenciadorDados gerenciadorDados;

    // Instancia a Simulação Física
    SimulacaoMina simulacao(mineGen.getMinefield(), 1); // 1 caminhão
    
    //Instancia o Driver de Simulação
    SimulacaoDriver simulacaoDriver(simulacao);

    // Inicializa o estado do veículo e os comandos do operador
    EstadoVeiculo estadoInicial = {false, false}; // Sem defeito, modo manual
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    ComandosOperador comandosIniciais = {false, true, false, false, false, false}; // Modo manual, sem outros comandos
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // Loop de Simulação Assíncrona da Mina

    std::function<void()> loop_sim_mina;
    
    loop_sim_mina = [&simulacao, &sleepAsynch, &loop_sim_mina]() {
        // 1. Executa a lógica
        simulacao.atualizar_passo_tempo();

        // 2. Agenda a próxima execução
        sleepAsynch.wait_next_tick(std::chrono::milliseconds(100), 
            [&loop_sim_mina](const boost::system::error_code& ec) {
                if (!ec) {
                    // Se não houve erro (ex: cancelamento), chama a função de novo
                    loop_sim_mina(); 
                }
            }
        );
    };

    // Dá o pontapé inicial no loop
    loop_sim_mina();

    // --- Thread para monitoramento do buffer ---
    std::thread t_monitor([&gerenciadorDados]() {
        while(true) {
            int tamanho = gerenciadorDados.getContadorDados();
            std::cout << "[BUFFER] Tamanho atual: " << tamanho << "/200" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // --- DEFINIÇÃO DAS THREADS ---

    // t_sim: roda o motor do ASIO. 
    // O io.run() vai ficar preso executando o loop_sim_mina indefinidamente.
    std::thread t_sim([&io]() {
        // O io_context.run() processa os timers e callbacks
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io.get_executor());
        io.run();
    });

    // t1 e t2 continuam iguais (elas são threads independentes)
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados), std::ref(simulacaoDriver), 0);
    // std::thread t2(task_logica_comando, std::ref(gerenciadorDados), std::ref(eventos));

    // t3: Monitoramento de Falhas (Lê sensores e envia eventos para a lógica de comando)
    // std::thread t3(task_monitoramento_falhas, std::ref(gerenciadorDados), std::ref(simulacao), std::ref(eventos));

    // Aguarda
    t_sim.join();
    t_monitor.join();
    t1.join();
    // t2.join();
    // t3.join();

    return 0;
}
