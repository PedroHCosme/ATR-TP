#ifndef INTERFACE_CAMINHAO_H
#define INTERFACE_CAMINHAO_H

#include <ncurses.h>
#include <string>
#include <vector>
#include "gerenciador_dados.h"
#include "eventos_sistema.h"

class InterfaceCaminhao {
private:
    GerenciadorDados& dados;
    EventosSistema& eventos;
    bool running;

    WINDOW* win_header;
    WINDOW* win_telemetry;
    WINDOW* win_status;
    WINDOW* win_controls;
    WINDOW* win_logs;

public:
    InterfaceCaminhao(GerenciadorDados& d, EventosSistema& e);
    ~InterfaceCaminhao();

    void init();
    void run();
    void close();

private:
    void draw_borders();
    void update_telemetry();
    void update_status();
    void update_controls();
    void handle_input();
};

#endif // INTERFACE_CAMINHAO_H
