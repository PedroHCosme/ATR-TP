#ifndef INTERFACE_SIMULACAO_H
#define INTERFACE_SIMULACAO_H

#include <ncurses.h>
#include <vector>
#include <mutex>
#include "simulacao_mina.h"
#include "gerenciador_dados.h"
#include "eventos_sistema.h"

/**
 * @brief Interface TUI (Text User Interface) para visualização e controle da simulação.
 * 
 * Utiliza a biblioteca ncurses para desenhar o mapa da mina, a posição do caminhão
 * e permitir a injeção de falhas ou comandos de simulação via teclado.
 */
class InterfaceSimulacao {
private:
    SimulacaoMina& simulacao;
    GerenciadorDados& dados;
    EventosSistema& eventos;
    const std::vector<std::vector<char>>& mapa;
    
    bool rodando;
    std::mutex mtx_tui;

public:
    /**
     * @brief Construtor da Interface.
     * 
     * @param sim Referência para o motor de física (para obter Ground Truth).
     * @param gerDados Referência para os dados do sistema (para ver estado do software).
     * @param evts Referência para o sistema de eventos (para injetar falhas).
     * @param mapaGrid Referência para o grid do mapa.
     */
    InterfaceSimulacao(SimulacaoMina& sim, GerenciadorDados& gerDados, EventosSistema& evts, const std::vector<std::vector<char>>& mapaGrid);

    ~InterfaceSimulacao();

    /**
     * @brief Inicializa o ncurses e as cores.
     */
    void init();

    /**
     * @brief Loop principal da interface.
     * Deve ser chamado na thread principal ou numa thread dedicada.
     * Atualiza a tela e processa inputs.
     */
    void run();

    /**
     * @brief Encerra a interface e restaura o terminal.
     */
    void close();

private:
    void desenharMapa();
    void desenharStatus();
    void processarInput();
};

#endif // INTERFACE_SIMULACAO_H
