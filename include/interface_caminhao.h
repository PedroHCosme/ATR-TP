#ifndef INTERFACE_CAMINHAO_H
#define INTERFACE_CAMINHAO_H

#include "eventos_sistema.h"
#include "gerenciador_dados.h"
#include "ipc_shared.h"
#include <boost/asio.hpp>
#include <ncurses.h>
#include <string>
#include <vector>

class InterfaceCaminhao {
private:
  bool running;
  GerenciadorDados &dados; // Reference to local data manager

  // Local Data Cache
  DadosSensores cached_sensores;
  EstadoVeiculo cached_estado;
  ComandosOperador current_commands;

  WINDOW *win_header;
  WINDOW *win_telemetry;
  WINDOW *win_status;
  WINDOW *win_controls;
  WINDOW *win_logs;

public:
  InterfaceCaminhao(int truck_id, GerenciadorDados &d);
  ~InterfaceCaminhao();

  void init();
  void init_windows(); // Helper to create/recreate windows
  void run();
  void close();
  void draw_borders();
  // void switch_truck(int truck_id); // Removed: Embedded cockpit is bound to
  // one truck

private:
  int current_truck_id;
  // void connect_to_truck(int truck_id); // Removed

private:
  void update_telemetry();
  void update_status();
  void update_controls();
  void handle_input();
};

#endif // INTERFACE_CAMINHAO_H
