#include <iostream>
#include <thread>
#include "gerenciador_dados.h"
#include "task_tratamento_sensores.h"
#include "task_logica_comando.h"
#include "mine_generator.h"

/**
 * @brief Função principal do sistema.
 * 
 * Inicializa o gerenciador de dados, configura o estado inicial do veículo
 * e inicia as threads responsáveis pelas tarefas do sistema.
 * 
 * @return int Código de saída do programa.
 */
int main() {
    // Gera e imprime o mapa da mina
    std::cout << "Gerando mapa da mina..." << std::endl;
    MineGenerator mineGen(21, 21); // Tamanho 21x21
    mineGen.generate();
    mineGen.print();
    std::cout << "Mapa gerado com sucesso!" << std::endl;

    // Instancia o gerenciador de dados compartilhado
    GerenciadorDados gerenciadorDados;

    // Inicializa o estado do veículo e os comandos do operador
    EstadoVeiculo estadoInicial = {false, false}; // Sem defeito, modo manual
    gerenciadorDados.setEstadoVeiculo(estadoInicial);

    ComandosOperador comandosIniciais = {false, true, false, false, false, false}; // Modo manual, sem outros comandos
    gerenciadorDados.setComandosOperador(comandosIniciais);

    // Cria e inicia as threads
    // t1: Produtora de dados (Lê sensores e escreve no buffer)
    // Passamos o mapa gerado para a task de sensores para detecção de colisão
    std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados), std::cref(mineGen.getMinefield()));
    
    // t2: Consumidora de dados (Lê do buffer e toma decisões)
    std::thread t2(task_logica_comando, std::ref(gerenciadorDados));

    // Aguarda as threads terminarem (loop infinito nas tasks, então o main fica bloqueado aqui)
    t1.join();
    t2.join();

    return 0;
}

