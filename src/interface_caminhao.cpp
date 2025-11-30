#include "interface_caminhao.h"
#include <boost/asio.hpp>
#include <chrono> // Required for std::chrono::seconds
#include <chrono>
#include <iostream>
#include <thread>
#include <thread> // Required for std::this_thread::sleep_for
#include <unistd.h>

InterfaceCaminhao::InterfaceCaminhao(int truck_id)
    : running(true), current_truck_id(truck_id), tcp_socket(nullptr) {
  // Inicializa ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE); // Non-blocking input

  win_header = nullptr;
  win_telemetry = nullptr;
  win_status = nullptr;
  win_controls = nullptr;
  win_logs = nullptr;

  // Initialize cache
  cached_sensores = {0};
  cached_estado = {0};
  current_commands = {0};

  connect_to_truck(current_truck_id);
}

void InterfaceCaminhao::connect_to_truck(int truck_id) {
  using boost::asio::ip::tcp;

  if (tcp_socket) {
    tcp_socket->close();
    delete tcp_socket;
    tcp_socket = nullptr;
  }
  tcp_socket = new tcp::socket(io_context);

  int port = BASE_PORT + truck_id;
  int retries = 0;

  while (true) {
    try {
      tcp_socket->connect(tcp::endpoint(
          boost::asio::ip::address::from_string("127.0.0.1"), port));
      break; // Connected!
    } catch (std::exception &e) {
      // Ignore and retry
    }

    mvprintw(0, 0, "Conectando ao Truck %d (Porta %d)... Tentativa %d",
             truck_id, port, ++retries);
    refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (retries > 20) {
      if (retries % 10 == 0) {
        mvprintw(1, 0, "Verifique se o bin/app para o Truck %d esta rodando.",
                 truck_id);
        refresh();
      }
    }

    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      running = false;
      return;
    }
  }
  clear();
}

void InterfaceCaminhao::switch_truck(int truck_id) {
  current_truck_id = truck_id;
  connect_to_truck(current_truck_id);
  draw_borders();
}

InterfaceCaminhao::~InterfaceCaminhao() { close(); }

void InterfaceCaminhao::init() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100); // Non-blocking getch (100ms delay)

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // Header
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Normal/Ok
    init_pair(3, COLOR_RED, COLOR_BLACK);    // Alert/Fault
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Warning/Info
    init_pair(5, COLOR_CYAN, COLOR_BLACK);   // Labels
  }

  // Layout Calculation
  int h, w;
  getmaxyx(stdscr, h, w);

  // Header: Top 3 lines
  win_header = newwin(3, w, 0, 0);
  wbkgd(win_header, COLOR_PAIR(1));

  // Telemetry: Left side
  win_telemetry = newwin(10, w / 2, 3, 0);

  // Status: Right side
  win_status = newwin(10, w / 2, 3, w / 2);

  // Controls: Bottom
  win_controls = newwin(h - 13, w, 13, 0);

  refresh();
}

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
  if (!win_header || !win_telemetry || !win_status || !win_controls) {
    endwin();
    std::cerr << "Erro: Terminal muito pequeno para a interface!" << std::endl;
    return;
  }

  draw_borders();    // Desenha a estrutura estática uma única vez
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
  if (!win_header)
    return;
  // Header
  wclear(win_header);
  box(win_header, 0, 0);
  mvwprintw(win_header, 1, 2, "SISTEMA DE CONTROLE - CAMINHAO %d (COCKPIT)",
            current_truck_id);
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
  // Read from Socket
  if (tcp_socket && tcp_socket->is_open()) {
    try {
      if (tcp_socket->available() > 0) {
        ServerToClientPacket rx_packet;
        boost::system::error_code error;
        size_t len = tcp_socket->read_some(
            boost::asio::buffer(&rx_packet, sizeof(rx_packet)), error);
        if (!error && len == sizeof(rx_packet)) {
          cached_sensores = rx_packet.sensores;
          cached_estado = rx_packet.estado;
        }
      }
    } catch (...) {
    }
  }

  DadosSensores sensores = cached_sensores;

  // Atualiza apenas os valores (com padding para limpar lixo anterior)
  wattron(win_telemetry, A_BOLD);

  wattron(win_telemetry, COLOR_PAIR(2));
  mvwprintw(win_telemetry, 2, 15, "%3d m/s   ", sensores.i_velocidade);
  wattroff(win_telemetry, COLOR_PAIR(2));

  if (sensores.i_temperatura > 90)
    wattron(win_telemetry, COLOR_PAIR(3));
  else
    wattron(win_telemetry, COLOR_PAIR(2));
  mvwprintw(win_telemetry, 3, 15, "%3d C     ", sensores.i_temperatura);
  wattroff(win_telemetry, COLOR_PAIR(3) | COLOR_PAIR(2));

  mvwprintw(win_telemetry, 4, 15, "%3d m     ", sensores.i_posicao_x);
  mvwprintw(win_telemetry, 5, 15, "%3d m     ", sensores.i_posicao_y);

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

    // Check for Obstacle specifically (using Lidar data as proxy since we don't
    // have fault code passed here easily yet) Ideally we would check the fault
    // code from 'eventos', but 'estado.e_defeito' is a bool. Let's check the
    // Lidar distance from 'dados'.
    if (sensores.i_lidar_distancia < 8.0f) { // SAFE_DISTANCE
      mvwprintw(win_status, 6, 22, "OBSTACULO DETECTADO!");
    }
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
  mvwprintw(win_controls, y++, x,
            "[ Q ] - Sair do Cockpit (Encerra Simulacao)");
  mvwprintw(win_controls, y++, x, "[ 0-2 ] - Alternar Caminhao");

  wnoutrefresh(win_controls);
}

void InterfaceCaminhao::handle_input() {
  int ch = getch();

  ComandosOperador cmd = current_commands;

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
      // Freio
      cmd.c_acelerar = false;
      break;
    case '0':
      switch_truck(0);
      return; // Skip writing to old SHM
    case '1':
      switch_truck(1);
      return;
    case '2':
      switch_truck(2);
      return;
    }
  }

  current_commands = cmd;

  // Write to Socket (Send heartbeat/commands)
  if (tcp_socket && tcp_socket->is_open()) {
    try {
      ClientToServerPacket tx_packet;
      tx_packet.comandos = current_commands;
      boost::system::error_code ignored_error;
      boost::asio::write(*tcp_socket,
                         boost::asio::buffer(&tx_packet, sizeof(tx_packet)),
                         ignored_error);
    } catch (...) {
    }
  }
}
