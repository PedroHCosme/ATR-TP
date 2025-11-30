#include "task_planejamento_rota.h"
#include "utils/sleep_asynch.h"
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Simple Point Struct for the internal queue
struct Point {
  float x, y;
  float speed;
};

void task_planejamento_rota(GerenciadorDados &dados, MqttDriver &mqtt) {
  boost::asio::io_context io;
  SleepAsynch timer(io);

  // Internal Route Queue (The "Mission")
  std::deque<Point> rota;

  std::cout << "[PLANNER] Task de Planejamento de Rota INICIADA." << std::endl;

  // Hardcoded mission for Phase 1 testing (A square lap)

  // Hardcoded mission for Phase 1 testing (A square lap)
  rota.push_back({100, 0, 20});
  rota.push_back({100, 100, 20});
  rota.push_back({0, 100, 20});
  rota.push_back({0, 0, 0}); // Stop at home

  // Acceptance radius (in meters)
  const float RAIO_CHEGADA = 5.0f;

  std::function<void()> loop_planejamento;
  loop_planejamento = [&]() {
    // 1. Get Current Position (Snapshot)
    auto estado = dados.lerUltimoEstado();
    float pos_x = static_cast<float>(estado.i_posicao_x);
    float pos_y = static_cast<float>(estado.i_posicao_y);

    // 2. Check MQTT for new missions (Phase 2 feature)
    // 2. Check MQTT for new missions
    std::string nova_missao = mqtt.checkNewRoute();
    if (!nova_missao.empty()) {
      try {
        auto j = json::parse(nova_missao);
        if (j.contains("route") && j["route"].is_array()) {
          rota.clear();
          std::cout << "[PLANNER] Nova rota recebida com " << j["route"].size()
                    << " pontos." << std::endl;
          for (const auto &p : j["route"]) {
            float x = p["x"];
            float y = p["y"];
            float speed = p.contains("speed") ? (float)p["speed"] : 20.0f;
            rota.push_back({x, y, speed});
          }
        }
      } catch (const std::exception &e) {
        std::cerr << "[PLANNER] Erro ao parsear rota: " << e.what()
                  << std::endl;
      }
    }

    // 3. Logic: Update Objective
    ObjetivoNavegacao novoObjetivo;

    if (rota.empty()) {
      // Mission Complete
      novoObjetivo = {false, 0, 0, 0};
    } else {
      // Get current target
      Point alvo = rota.front();

      // Calculate Distance (Euclidean)
      float dx = alvo.x - pos_x;
      float dy = alvo.y - pos_y;
      float dist = std::sqrt(dx * dx + dy * dy);

      // Are we there yet?
      if (dist < RAIO_CHEGADA) {
        std::cout << "[PLANNER] Waypoint reached! (" << alvo.x << "," << alvo.y
                  << ")" << std::endl;
        rota.pop_front(); // Remove current, move to next

        // If route finished, stop immediately
        if (rota.empty()) {
          novoObjetivo = {false, 0, 0, 0};
        } else {
          alvo = rota.front();
          novoObjetivo = {true, alvo.x, alvo.y, alvo.speed};
        }
      } else {
        // Keep going to current target
        novoObjetivo = {true, alvo.x, alvo.y, alvo.speed};
      }
    }

    // 4. Write to Shared Memory (Direct Memory Access)
    dados.setObjetivo(novoObjetivo);

    // Run at 10Hz (Planning doesn't need to be as fast as Control)
    timer.wait_next_tick(std::chrono::milliseconds(100),
                         [&](const boost::system::error_code &ec) {
                           if (!ec)
                             loop_planejamento();
                         });
  };

  loop_planejamento();
  io.run();
}
