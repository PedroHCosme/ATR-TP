#include "interface_caminhao.h"
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <iostream>

InterfaceCaminhao::InterfaceCaminhao(GerenciadorDados& d, EventosSistema& e)
    : dados(d), eventos(e), running(false) {
    win_header = nullptr;
    win_telemetry = nullptr;
    win_status = nullptr;
    win_controls = nullptr;
    win_logs = nullptr;
}

InterfaceCaminhao::~InterfaceCaminhao() {
    close();
}

void InterfaceCaminhao::init() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100); // Non-blocking getch (100ms delay)

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);  // Header
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Normal/Ok
        init_pair(3, COLOR_RED, COLOR_BLACK);   // Alert/Fault
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);// Warning/Info
        init_pair(5, COLOR_CYAN, COLOR_BLACK);  // Labels
    }

    // Layout Calculation
    int h, w;
    getmaxyx(stdscr, h, w);

    // Header: Top 3 lines
    win_header = newwin(3, w, 0, 0);
    wbkgd(win_header, COLOR_PAIR(1));

    // Telemetry: Left side
    win_telemetry = newwin(10, w/2, 3, 0);
    
    // Status: Right side
    win_status = newwin(10, w/2, 3, w/2);

    // Controls: Bottom
    win_controls = newwin(h - 13, w, 13, 0);

    refresh();
}

void InterfaceCaminhao::close() {
    if (win_header) delwin(win_header);
    if (win_telemetry) delwin(win_telemetry);
    if (win_status) delwin(win_status);
    if (win_controls) delwin(win_controls);
    endwin();
}

void InterfaceCaminhao::run() {
    if (!win_header || !win_telemetry || !win_status || !win_controls) {
        endwin();
        std::cerr << "Erro: Terminal muito pequeno para a interface!" << std::endl;
        return;
    }

    draw_borders(); // Desenha a estrutura estática uma única vez
    update_controls(); // Controles são estáticos

    running = true;
    while (running) {
        // Apenas atualiza os dados dinâmicos
        update_telemetry();
        update_status();
        handle_input();
        
        doupdate();
        // napms(50); // Handled by timeout in getch
    }
}

void InterfaceCaminhao::draw_borders() {
    if (!win_header) return;
    // Header
    wclear(win_header);
    box(win_header, 0, 0);
    mvwprintw(win_header, 1, 2, "SISTEMA DE CONTROLE - CAMINHAO AUTONOMO (COCKPIT)");
    wnoutrefresh(win_header);

    // Telemetry
    wclear(win_telemetry);
    box(win_telemetry, 0, 0);
    wattron(win_telemetry, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_telemetry, 0, 2, " TELEMETRIA ");
    wattroff(win_telemetry, COLOR_PAIR(5) | A_BOLD);
    
    // Labels Estáticos da Telemetria
    wattron(win_telemetry, A_BOLD);
    mvwprintw(win_telemetry, 2, 2, "Velocidade:");
    mvwprintw(win_telemetry, 3, 2, "Temperatura:");
    mvwprintw(win_telemetry, 4, 2, "Posicao X:");
    mvwprintw(win_telemetry, 5, 2, "Posicao Y:");
    wattroff(win_telemetry, A_BOLD);
    
    wnoutrefresh(win_telemetry);

    // Status
    wclear(win_status);
    box(win_status, 0, 0);
    wattron(win_status, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_status, 0, 2, " STATUS DO SISTEMA ");
    wattroff(win_status, COLOR_PAIR(5) | A_BOLD);
    
    // Labels Estáticos do Status
    mvwprintw(win_status, 2, 2, "Modo de Operacao:");
    mvwprintw(win_status, 4, 2, "Integridade:");
    
    wnoutrefresh(win_status);

    // Controls
    wclear(win_controls);
    box(win_controls, 0, 0);
    wattron(win_controls, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_controls, 0, 2, " GUIA DE COMANDOS ");
    wattroff(win_controls, COLOR_PAIR(5) | A_BOLD);
    wnoutrefresh(win_controls);
}

void InterfaceCaminhao::update_telemetry() {
    // Acessando via DadosSensores
    DadosSensores sensores = dados.lerUltimoEstado();

    // Atualiza apenas os valores (com padding para limpar lixo anterior)
    wattron(win_telemetry, A_BOLD);
    
    wattron(win_telemetry, COLOR_PAIR(2));
    mvwprintw(win_telemetry, 2, 15, "%3d m/s   ", sensores.i_velocidade); 
    wattroff(win_telemetry, COLOR_PAIR(2));

    if (sensores.i_temperatura > 90) wattron(win_telemetry, COLOR_PAIR(3));
    else wattron(win_telemetry, COLOR_PAIR(2));
    mvwprintw(win_telemetry, 3, 15, "%3d C     ", sensores.i_temperatura);
    wattroff(win_telemetry, COLOR_PAIR(3) | COLOR_PAIR(2));

    mvwprintw(win_telemetry, 4, 15, "%3d m     ", sensores.i_posicao_x);
    mvwprintw(win_telemetry, 5, 15, "%3d m     ", sensores.i_posicao_y);

    wattroff(win_telemetry, A_BOLD);
    wnoutrefresh(win_telemetry);
}

void InterfaceCaminhao::update_status() {
    EstadoVeiculo estado = dados.getEstadoVeiculo();
    ComandosOperador cmd = dados.getComandosOperador();

    if (estado.e_automatico) {
        wattron(win_status, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(win_status, 2, 22, "AUTOMATICO   ");
        wattroff(win_status, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(win_status, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(win_status, 2, 22, "MANUAL       ");
        wattroff(win_status, COLOR_PAIR(4) | A_BOLD);
    }

    if (estado.e_defeito) {
        wattron(win_status, COLOR_PAIR(3) | A_BLINK | A_BOLD);
        mvwprintw(win_status, 4, 22, "FALHA DETECTADA     ");
        wattroff(win_status, COLOR_PAIR(3) | A_BLINK | A_BOLD);
        
        mvwprintw(win_status, 5, 2, "Acao:");
        wattron(win_status, COLOR_PAIR(3));
        mvwprintw(win_status, 5, 22, "PARADA DE EMERGENCIA");
        wattroff(win_status, COLOR_PAIR(3));
    } else {
        wattron(win_status, COLOR_PAIR(2));
        mvwprintw(win_status, 4, 22, "SISTEMA OK          ");
        wattroff(win_status, COLOR_PAIR(2));
        
        // Limpa linha de ação se não houver falha
        mvwprintw(win_status, 5, 2, "                    "); 
        mvwprintw(win_status, 5, 22, "                    ");
    }

    if (cmd.c_rearme) {
        wattron(win_status, COLOR_PAIR(4));
        mvwprintw(win_status, 7, 2, ">> REARMANDO SISTEMA <<");
        wattroff(win_status, COLOR_PAIR(4));
    } else {
        mvwprintw(win_status, 7, 2, "                       ");
    }

    wnoutrefresh(win_status);
}

void InterfaceCaminhao::update_controls() {
    int y = 2;
    int x = 4;
    
    wattron(win_controls, A_BOLD);
    mvwprintw(win_controls, y++, x, "COMANDOS DISPONIVEIS:");
    wattroff(win_controls, A_BOLD);
    y++;

    mvwprintw(win_controls, y++, x, "[ A ] - Ativar Modo AUTOMATICO");
    mvwprintw(win_controls, y++, x, "[ M ] - Ativar Modo MANUAL");
    mvwprintw(win_controls, y++, x, "[ R ] - REARMAR Sistema (Resetar Falhas)");
    y++;
    mvwprintw(win_controls, y++, x, "CONTROLE MANUAL:");
    mvwprintw(win_controls, y++, x, "[ SETA CIMA ]    - Acelerar");
    mvwprintw(win_controls, y++, x, "[ SETA ESQ/DIR ] - Virar");
    mvwprintw(win_controls, y++, x, "[ ESPACO ]       - Freio de Emergencia");
    y++;
    mvwprintw(win_controls, y++, x, "[ Q ] - Sair do Cockpit (Encerra Simulacao)");

    wnoutrefresh(win_controls);
}

void InterfaceCaminhao::handle_input() {
    int ch = getch();
    
    ComandosOperador cmd = dados.getComandosOperador();

    // Reset de comandos momentâneos no modo manual
    // Se nenhuma tecla for pressionada (ch == ERR), ou se for outra tecla,
    // assumimos que o operador soltou o acelerador/volante.
    if (cmd.c_man) {
        cmd.c_acelerar = false;
        cmd.c_esquerda = false;
        cmd.c_direita = false;
        cmd.c_rearme = false;
    }

    if (ch != ERR) {
        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;
            case 'a':
            case 'A':
                cmd.c_automatico = true;
                cmd.c_man = false;
                break;
            case 'm':
            case 'M':
                cmd.c_automatico = false;
                cmd.c_man = true;
                break;
            case 'r':
            case 'R':
                cmd.c_rearme = true;
                break;
            case KEY_UP:
                if (cmd.c_man) cmd.c_acelerar = true;
                break;
            case KEY_LEFT:
                if (cmd.c_man) cmd.c_esquerda = true;
                break;
            case KEY_RIGHT:
                if (cmd.c_man) cmd.c_direita = true;
                break;
            case ' ':
                // Freio
                cmd.c_acelerar = false;
                break;
        }
    }

    dados.setComandosOperador(cmd);
}
