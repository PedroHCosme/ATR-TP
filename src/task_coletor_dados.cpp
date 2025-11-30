#include "task_coletor_dados.h"
#include "utils/sleep_asynch.h"
#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

void task_coletor_dados(GerenciadorDados &dados, EventosSistema &eventos) {
  boost::asio::io_context io;
  SleepAsynch timer(io);

  // Ensure logs directory exists
  mkdir("logs", 0777);

  std::string log_file_path = "logs/operation.log";
  std::ofstream log_file;

  // Open file in append mode
  log_file.open(log_file_path, std::ios::app);
  if (!log_file.is_open()) {
    std::cerr << "[Coletor] Erro ao abrir arquivo de log: " << log_file_path
              << std::endl;
    return;
  }

  std::function<void()> loop_coletor;
  loop_coletor = [&]() {
    // 1. Get Data
    DadosSensores sensores = dados.lerUltimoEstado();
    EstadoVeiculo estado = dados.getEstadoVeiculo();

    // 2. Format Timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss_time;
    ss_time << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    // 3. Determine State String
    std::string mode = estado.e_automatico ? "AUTO" : "MANUAL";
    std::string status = estado.e_defeito ? "FALHA" : "NORMAL";

    // 4. Write to File
    // Format: [Timestamp] | ID | Mode | Status | X | Y | Vel | Temp
    log_file << "[" << ss_time.str() << "] | "
             << "ID: " << sensores.id << " | "
             << "Mode: " << mode << " | "
             << "Status: " << status << " | "
             << "Pos: (" << std::fixed << std::setprecision(1)
             << sensores.i_posicao_x << ", " << sensores.i_posicao_y << ") | "
             << "Vel: " << sensores.i_velocidade << " | "
             << "Temp: " << sensores.i_temperatura << std::endl;

    // Flush to ensure data is written to disk
    log_file.flush();

    // 5. Schedule Next Tick (1Hz is usually enough for logging, but let's do
    // 5Hz)
    timer.wait_next_tick(std::chrono::milliseconds(200),
                         [&](const boost::system::error_code &ec) {
                           if (!ec)
                             loop_coletor();
                         });
  };

  loop_coletor();
  io.run();
}
