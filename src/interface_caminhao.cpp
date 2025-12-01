#include "interface_caminhao.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

InterfaceCaminhao::InterfaceCaminhao(int truck_id, GerenciadorDados &d)
    : running(true), dados(d), current_truck_id(truck_id) {

  // Initialize ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // Header
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Normal/Ok
    init_pair(3, COLOR_RED, COLOR_BLACK);    // Alert/Fault
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Warning/Info
    init_pair(5, COLOR_CYAN, COLOR_BLACK);   // Labels
  }

  // Initialize windows
  init_windows();

  // Initialize cache
  cached_sensores = {0};
  cached_estado = {0};
  current_commands = {0};
}

void InterfaceCaminhao::init_windows() {
  int h, w;
  getmaxyx(stdscr, h, w);

  if (win_header)
    delwin(win_header);
  if (win_telemetry)
    delwin(win_telemetry);
  if (win_status)
    delwin(win_status);
  if (win_controls)
    delwin(win_controls);

  // Header: Top 3 lines
  win_header = newwin(3, w, 0, 0);
  wbkgd(win_header, COLOR_PAIR(1));

  // Telemetry: Left side (50% width)
  win_telemetry = newwin(10, w / 2, 3, 0);

  // Status: Right side (50% width)
  win_status = newwin(10, w - (w / 2), 3, w / 2);

  // Controls: Bottom (Remaining height)
  win_controls = newwin(h - 13, w, 13, 0);

  refresh();
  nodelay(stdscr, TRUE); // Ensure non-blocking input persists
  draw_borders();
  update_controls();
}

InterfaceCaminhao::~InterfaceCaminhao() { close(); }

void InterfaceCaminhao::close() {
  if (win_header)
    delwin(win_header);
  if (win_telemetry)
    delwin(win_telemetry);
  if (win_status)
    delwin(win_status);
  if (win_controls)
    delwin(win_controls);
  endwin();
}

void InterfaceCaminhao::run() {
  running = true;
  int frame_count = 0;

  while (running) {
    // Periodic Redraw (every 20 frames ~ 1 second)
    if (frame_count % 20 == 0) {
      draw_borders();
      update_controls();
    }
    frame_count++;

    update_telemetry();
    update_status();
    handle_input();

    doupdate();
    napms(50); // Control frame rate (~20Hz)
  }
}

void InterfaceCaminhao::draw_borders() {
  if (!win_header)
    return;

  // Header
  wclear(win_header);
  box(win_header, 0, 0);
  mvwprintw(win_header, 1, 2,
            "SISTEMA DE CONTROLE - CAMINHAO %d (COCKPIT INTEGRADO)",
            current_truck_id);
  wrefresh(win_header);

  // Telemetry
  wclear(win_telemetry);
  box(win_telemetry, 0, 0);
  wattron(win_telemetry, COLOR_PAIR(5) | A_BOLD);
  mvwprintw(win_telemetry, 0, 2, " TELEMETRIA ");
  wattroff(win_telemetry, COLOR_PAIR(5) | A_BOLD);

  wattron(win_telemetry, A_BOLD);
  mvwprintw(win_telemetry, 2, 2, "Velocidade:");
  mvwprintw(win_telemetry, 3, 2, "Temperatura:");
  mvwprintw(win_telemetry, 4, 2, "Posicao X:");
  mvwprintw(win_telemetry, 5, 2, "Posicao Y:");
  wattroff(win_telemetry, A_BOLD);
  wrefresh(win_telemetry);

  // Status
  wclear(win_status);
  box(win_status, 0, 0);
  wattron(win_status, COLOR_PAIR(5) | A_BOLD);
  mvwprintw(win_status, 0, 2, " STATUS DO SISTEMA ");
  wattroff(win_status, COLOR_PAIR(5) | A_BOLD);

  mvwprintw(win_status, 2, 2, "Modo de Operacao:");
  mvwprintw(win_status, 4, 2, "Integridade:");

  // DEBUG INFO
  mvwprintw(win_status, 8, 2, "Mode: DIRECT BUFFER ACCESS");

  wrefresh(win_status);

  // Controls
  wclear(win_controls);
  box(win_controls, 0, 0);
  wattron(win_controls, COLOR_PAIR(5) | A_BOLD);
  mvwprintw(win_controls, 0, 2, " GUIA DE COMANDOS ");
  wattroff(win_controls, COLOR_PAIR(5) | A_BOLD);
  wrefresh(win_controls);
}

void InterfaceCaminhao::update_telemetry() {
  // DIRECT ACCESS: Read from GerenciadorDados
  cached_sensores = dados.lerUltimoEstado();
  cached_estado = dados.getEstadoVeiculo();

  DadosSensores sensores = cached_sensores;

  wattron(win_telemetry, A_BOLD);
  wattron(win_telemetry, COLOR_PAIR(2));
  mvwprintw(win_telemetry, 2, 15, "%3d m/s", sensores.i_velocidade);
  wattroff(win_telemetry, COLOR_PAIR(2));

  if (sensores.i_temperatura > 90)
    wattron(win_telemetry, COLOR_PAIR(3));
  else
    wattron(win_telemetry, COLOR_PAIR(2));
  mvwprintw(win_telemetry, 3, 15, "%3d C", sensores.i_temperatura);
  wattroff(win_telemetry, COLOR_PAIR(3) | COLOR_PAIR(2));

  mvwprintw(win_telemetry, 4, 15, "%3d m", sensores.i_posicao_x);
  mvwprintw(win_telemetry, 5, 15, "%3d m", sensores.i_posicao_y);
  wattroff(win_telemetry, A_BOLD);
  wnoutrefresh(win_telemetry);
}

void InterfaceCaminhao::update_status() {
  EstadoVeiculo estado = cached_estado;
  ComandosOperador cmd = current_commands;
  DadosSensores sensores = cached_sensores;

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

    if (sensores.i_lidar_distancia < 8.0f) {
      mvwprintw(win_status, 6, 22, "OBSTACULO DETECTADO!");
    }
    wattroff(win_status, COLOR_PAIR(3));
  } else {
    wattron(win_status, COLOR_PAIR(2));
    mvwprintw(win_status, 4, 22, "SISTEMA OK          ");
    wattroff(win_status, COLOR_PAIR(2));
    mvwprintw(win_status, 5, 2, "                    ");
    mvwprintw(win_status, 5, 22, "                    ");
    mvwprintw(win_status, 6, 22, "                    ");
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
  mvwprintw(win_controls, y++, x, "[ Q ] - Sair do Cockpit");
  // mvwprintw(win_controls, y++, x, "[ 0-2 ] - Alternar Caminhao"); // Removed

  wnoutrefresh(win_controls);
}

void InterfaceCaminhao::handle_input() {
  int ch = getch();
  ComandosOperador cmd = current_commands;

  // Reset transient commands
  cmd.c_rearme = false; // Always reset rearm after one frame

  // Reset manual controls if no key pressed
  if (cmd.c_man) {
    cmd.c_acelerar = false;
    cmd.c_esquerda = false;
    cmd.c_direita = false;
  }

  if (ch != ERR) {
    switch (ch) {
    case KEY_RESIZE:
      endwin();
      refresh();      // Restore ncurses state
      init_windows(); // Recalculate layout
      break;
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
      if (cmd.c_man)
        cmd.c_acelerar = true;
      break;
    case KEY_LEFT:
      if (cmd.c_man)
        cmd.c_esquerda = true;
      break;
    case KEY_RIGHT:
      if (cmd.c_man)
        cmd.c_direita = true;
      break;
    case ' ':
      cmd.c_acelerar = false;
      break;
    }
  }

  current_commands = cmd;

  // DIRECT ACCESS: Write to GerenciadorDados
  dados.setComandosOperador(current_commands);
}
