#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mosquitto.h>
#include <ncurses.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

// Global state
struct TruckState {
  int id;
  float x, y;
  float velocity;
  int temperature;
  float lidar;
  bool fault;
  bool is_auto;
};

std::atomic<bool> running(true);
std::atomic<bool> connected(false);
std::map<int, TruckState> trucks;
int map_width = 0;
int map_height = 0;
int route_waypoints = 0;

// MQTT Callbacks
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
  if (rc == 0) {
    connected = true;
    mosquitto_subscribe(mosq, NULL, "caminhao/sensores", 0);
    mosquitto_subscribe(mosq, NULL, "caminhao/mapa", 0);
    mosquitto_subscribe(mosq, NULL, "caminhao/rota", 0);
    mosquitto_subscribe(mosq, NULL, "caminhao/estado_sistema", 0);
  } else {
    connected = false;
  }
}

void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
  std::string topic = msg->topic;
  std::string payload(static_cast<char *>(msg->payload), msg->payloadlen);

  try {
    auto j = json::parse(payload);

    if (topic == "caminhao/sensores") {
      int id = j["id"];
      TruckState &t = trucks[id];
      t.id = id;
      t.x = j["x"];
      t.y = j["y"];
      t.velocity = j["vel"];
      t.temperature = j["temp"];
      // t.fault = j["fault"]; // Fault comes from estado_sistema now
      if (j.contains("lidar"))
        t.lidar = j["lidar"];
      else
        t.lidar = 0.0f;
      // if (j.contains("auto")) t.is_auto = j["auto"]; // Auto comes from
      // estado_sistema

    } else if (topic == "caminhao/estado_sistema") {
      // This topic currently doesn't have ID, assume ID 0 or apply to all known
      // For now, let's update ID 0 if it exists, or create it.
      // Better: Update all trucks or just the primary one.
      // Let's assume ID 0 is the main one controlled by the interface.
      TruckState &t = trucks[0];
      t.id = 0; // Ensure it exists
      if (j.contains("manual"))
        t.is_auto = !j["manual"].get<bool>();
      if (j.contains("fault"))
        t.fault = j["fault"].get<bool>();

    } else if (topic == "caminhao/mapa") {
      if (j.contains("width"))
        map_width = j["width"];
      if (j.contains("height"))
        map_height = j["height"];
    } else if (topic == "caminhao/rota") {
      if (j.contains("route") && j["route"].is_array()) {
        route_waypoints = j["route"].size();
      }
    }
  } catch (...) {
    // Ignore parsing errors
  }
}

void draw_interface() {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  timeout(100); // Non-blocking getch with 100ms delay

  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_BLACK);
  init_pair(5, COLOR_CYAN, COLOR_BLACK);
  init_pair(6, COLOR_BLACK, COLOR_WHITE); // Header row

  WINDOW *win = newwin(LINES, COLS, 0, 0);
  box(win, 0, 0);
  wbkgd(win, COLOR_PAIR(1));

  while (running) {
    werase(win);
    box(win, 0, 0);

    // --- Header Section ---
    wattron(win, A_BOLD | COLOR_PAIR(5));
    mvwprintw(win, 1, 2,
              "SISTEMA DE GESTAO DE MINA - MONITORAMENTO EM TEMPO REAL");
    wattroff(win, A_BOLD | COLOR_PAIR(5));

    // --- Info Bar ---
    int info_y = 3;
    mvwprintw(win, info_y, 2, "MQTT: ");
    if (connected) {
      wattron(win, COLOR_PAIR(2));
      wprintw(win, "CONECTADO");
      wattroff(win, COLOR_PAIR(2));
    } else {
      wattron(win, COLOR_PAIR(3));
      wprintw(win, "DESCONECTADO");
      wattroff(win, COLOR_PAIR(3));
    }

    mvwprintw(win, info_y, 25, "Dimensoes Mina: %dx%dm", map_width, map_height);
    mvwprintw(win, info_y, 55, "Rota Planejada: %d waypoints", route_waypoints);
    mvwprintw(win, info_y, 85, "Caminhoes Ativos: %lu", trucks.size());

    // --- Truck Table ---
    int table_y = 6;
    int col_id = 2;
    int col_pos = 8;
    int col_vel = 25;
    int col_temp = 38;
    int col_lidar = 50;
    int col_mode = 62;
    int col_status = 72;

    // Table Header
    wattron(win, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(win, table_y, col_id, " ID ");
    mvwprintw(win, table_y, col_pos, " POSICAO (X,Y) ");
    mvwprintw(win, table_y, col_vel, " VEL (m/s) ");
    mvwprintw(win, table_y, col_temp, " TEMP (C) ");
    mvwprintw(win, table_y, col_lidar, " LIDAR (m) ");
    mvwprintw(win, table_y, col_mode, " MODO ");
    mvwprintw(win, table_y, col_status, " STATUS       ");
    wattroff(win, COLOR_PAIR(6) | A_BOLD);

    // Table Rows
    int row = 0;
    for (const auto &pair : trucks) {
      const TruckState &t = pair.second;
      int y = table_y + 2 + row;

      if (y >= LINES - 2)
        break; // Prevent overflow

      mvwprintw(win, y, col_id, "#%d", t.id);
      mvwprintw(win, y, col_pos, "(%5.1f, %5.1f)", t.x, t.y);
      mvwprintw(win, y, col_vel, "%5.1f", t.velocity);

      // Temperature Color Logic
      if (t.temperature > 90)
        wattron(win, COLOR_PAIR(3));
      else if (t.temperature > 75)
        wattron(win, COLOR_PAIR(4));
      else
        wattron(win, COLOR_PAIR(2));
      mvwprintw(win, y, col_temp, "%3d", t.temperature);
      wattroff(win, COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));

      mvwprintw(win, y, col_lidar, "%5.1f", t.lidar);

      // Mode
      if (t.is_auto)
        mvwprintw(win, y, col_mode, "AUTO");
      else {
        wattron(win, COLOR_PAIR(4));
        mvwprintw(win, y, col_mode, "MANUAL");
        wattroff(win, COLOR_PAIR(4));
      }

      // Status
      if (t.fault) {
        wattron(win, A_BLINK | COLOR_PAIR(3));
        mvwprintw(win, y, col_status, "FALHA CRITICA");
        wattroff(win, A_BLINK | COLOR_PAIR(3));
      } else {
        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, y, col_status, "OPERACIONAL");
        wattroff(win, COLOR_PAIR(2));
      }

      row++;
    }

    // Footer
    mvwprintw(win, LINES - 2, 2,
              "Pressione 'q' para sair (apenas esta janela)");

    wrefresh(win);

    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      running = false;
    }
  }

  delwin(win);
  endwin();
}

int main() {
  mosquitto_lib_init();
  struct mosquitto *mosq = mosquitto_new("interface_simulacao", true, NULL);

  if (!mosq) {
    std::cerr << "Erro ao criar cliente MQTT" << std::endl;
    return 1;
  }

  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_message_callback_set(mosq, on_message);

  if (mosquitto_connect(mosq, "127.0.0.1", 1883, 60) != MOSQ_ERR_SUCCESS) {
    std::cerr << "Erro ao conectar ao broker" << std::endl;
    return 1;
  }

  mosquitto_loop_start(mosq);

  draw_interface();

  mosquitto_loop_stop(mosq, true);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  return 0;
}
