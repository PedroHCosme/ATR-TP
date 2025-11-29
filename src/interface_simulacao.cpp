#include "interface_simulacao.h"
#include <string>
#include <thread>
#include <chrono>

InterfaceSimulacao::InterfaceSimulacao(SimulacaoMina& sim, GerenciadorDados& gerDados, EventosSistema& evts, const std::vector<std::vector<char>>& mapaGrid)
    : simulacao(sim), dados(gerDados), eventos(evts), mapa(mapaGrid), rodando(true) {
}

InterfaceSimulacao::~InterfaceSimulacao() {
    close();
}

void InterfaceSimulacao::init() {
    initscr();            // Inicializa ncurses
    cbreak();             // Desabilita buffer de linha
    noecho();             // Não mostra teclas digitadas
    keypad(stdscr, TRUE); // Habilita teclas especiais (setas, F1, etc)
    curs_set(0);          // Esconde o cursor
    timeout(100);         // getch() espera 100ms (não bloqueante)

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK); // Padrão
        init_pair(2, COLOR_RED, COLOR_BLACK);   // Parede / Falha
        init_pair(3, COLOR_GREEN, COLOR_BLACK); // Caminhão Normal
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);// Caminho / Alerta
        init_pair(5, COLOR_CYAN, COLOR_BLACK);  // Info
    }
}

void InterfaceSimulacao::close() {
    endwin(); // Restaura o terminal
}

void InterfaceSimulacao::run() {
    while (rodando) {
        erase(); // Limpa a tela

        desenharMapa();
        desenharStatus();
        processarInput();

        refresh(); // Atualiza a tela
        
        // Pequeno sleep para não comer 100% da CPU da thread de UI
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void InterfaceSimulacao::desenharMapa() {
    // Desenha o grid
    // Assumindo que o mapa cabe na tela. Se for muito grande, precisaria de viewport.
    // O mapa é 61x61, pode ser grande para terminais pequenos.
    // Vamos desenhar um viewport centrado no caminhão ou fixo se couber.
    
    // Pegando estado real para desenhar o caminhão
    CaminhaoFisico caminhao = simulacao.getEstadoReal(0);
    int camX = static_cast<int>(caminhao.i_posicao_x);
    int camY = static_cast<int>(caminhao.i_posicao_y);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Offset para centralizar ou apenas desenhar a partir de (0,0)
    int startRow = 0;
    int startCol = 0;

    for (size_t y = 0; y < mapa.size(); ++y) {
        for (size_t x = 0; x < mapa[y].size(); ++x) {
            // Verifica se está dentro da tela
            if (y < (size_t)rows && x < (size_t)cols) {
                char cell = mapa[y][x];
                if (x == (size_t)camX && y == (size_t)camY) {
                    attron(COLOR_PAIR(3)); // Verde para o caminhão
                    mvaddch(y + startRow, x + startCol, 'C');
                    attroff(COLOR_PAIR(3));
                } else if (cell == '1') {
                    attron(COLOR_PAIR(2)); // Vermelho para parede
                    mvaddch(y + startRow, x + startCol, '#');
                    attroff(COLOR_PAIR(2));
                } else {
                    mvaddch(y + startRow, x + startCol, '.');
                }
            }
        }
    }
}

void InterfaceSimulacao::desenharStatus() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows; // Silencia warning de variável não usada

    // Painel lateral (se houver espaço) ou rodapé
    int startY = 0;
    int startX = mapa[0].size() + 2; // Ao lado do mapa

    if (startX < cols) {
        CaminhaoFisico real = simulacao.getEstadoReal(0);
        EstadoVeiculo estado = dados.getEstadoVeiculo();
        DadosSensores sensores = dados.lerUltimoEstado();

        attron(COLOR_PAIR(5));
        mvprintw(startY++, startX, "=== SIMULACAO MINA ===");
        mvprintw(startY++, startX, "Posicao Real: (%.2f, %.2f)", real.i_posicao_x, real.i_posicao_y);
        mvprintw(startY++, startX, "Velocidade: %.2f m/s", real.velocidade);
        mvprintw(startY++, startX, "Temperatura: %.2f C", real.i_temperatura);
        
        startY++;
        mvprintw(startY++, startX, "=== SISTEMA EMBARCADO ===");
        mvprintw(startY++, startX, "Modo: %s", estado.e_automatico ? "AUTOMATICO" : "MANUAL");
        mvprintw(startY++, startX, "Falha: %s", estado.e_defeito ? "SIM" : "NAO");
        
        if (sensores.i_falha_eletrica) {
            attron(COLOR_PAIR(2));
            mvprintw(startY++, startX, "[!] FALHA ELETRICA DETECTADA");
            attroff(COLOR_PAIR(2));
        }
        if (sensores.i_falha_hidraulica) {
            attron(COLOR_PAIR(2));
            mvprintw(startY++, startX, "[!] FALHA HIDRAULICA DETECTADA");
            attroff(COLOR_PAIR(2));
        }

        startY++;
        mvprintw(startY++, startX, "=== COMANDOS (Teclado) ===");
        mvprintw(startY++, startX, "[q] Sair");
        mvprintw(startY++, startX, "[e] Injetar Falha Eletrica");
        mvprintw(startY++, startX, "[h] Injetar Falha Hidraulica");
        mvprintw(startY++, startX, "[r] Resetar Falhas (Rearme)");
        attroff(COLOR_PAIR(5));
    }
}

void InterfaceSimulacao::processarInput() {
    int ch = getch(); // Non-blocking (timeout definido no init)

    if (ch != ERR) {
        switch (ch) {
            case 'q':
            case 'Q':
                rodando = false;
                break;
            case 'e': // Injetar Falha Elétrica
                // Aqui precisaríamos de um método na simulação para injetar falha no hardware
                // Como a simulação é simples, vamos forçar via EventosSistema ou criar um método na SimulacaoMina?
                // O correto seria a SimulacaoMina ter "setFalhaEletrica(true)".
                // Por enquanto, vamos sinalizar direto no sistema de eventos para testar a reação
                eventos.sinalizar_falha(2); 
                break;
            case 'h': // Injetar Falha Hidráulica
                eventos.sinalizar_falha(3);
                break;
            case 'r': // Rearme
                // Simula o operador apertando o botão de rearme
                ComandosOperador cmd = dados.getComandosOperador();
                cmd.c_rearme = true;
                dados.setComandosOperador(cmd);
                // Reseta o flag logo depois (pulso)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cmd.c_rearme = false;
                dados.setComandosOperador(cmd);
                break;
        }
    }
}
