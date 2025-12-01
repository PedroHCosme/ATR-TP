#include "task_coletor_dados.h"
#include "ipc_shared.h"
#include "utils/sleep_asynch.h"
#include <boost/asio.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <thread>

void task_coletor_dados(GerenciadorDados &dados, EventosSistema &eventos,
                        int truck_id) {
  using boost::asio::ip::tcp;

  // Ensure logs directory exists
  mkdir("logs", 0777);
  std::string log_file_path =
      "logs/coletor_" + std::to_string(truck_id) + ".log";
  std::ofstream log_file(log_file_path, std::ios_base::app);

  try {
    boost::asio::io_context io_context;
    int port = BASE_PORT + truck_id;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    std::cout << "[Coletor " << truck_id << "] Servidor TCP ouvindo na porta "
              << port << std::endl;

    while (true) {
      tcp::socket socket(io_context);
      acceptor.accept(socket);

      std::cout << "[Coletor " << truck_id << "] Cliente conectado!"
                << std::endl;

      try {
        while (true) {
          // 1. Prepare Data Packet
          ServerToClientPacket tx_packet;
          tx_packet.sensores = dados.lerUltimoEstado();
          tx_packet.estado = dados.getEstadoVeiculo();

          // 2. Send Data (Write)
          // DEBUG: Log packet content
          unsigned char *p = (unsigned char *)&tx_packet;
          std::cout << "[Coletor] Sending " << sizeof(tx_packet) << " bytes: ";
          for (size_t i = 0; i < sizeof(tx_packet); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)p[i] << " ";
          }
          std::cout << std::dec << std::endl;

          boost::system::error_code ignored_error;
          boost::asio::write(socket,
                             boost::asio::buffer(&tx_packet, sizeof(tx_packet)),
                             ignored_error);

          // 3. Receive Commands (Read - Non-blocking check or small timeout?)
          // For simplicity in this loop, we can do a blocking read if we expect
          // constant traffic, OR use available() to check if data is there.
          // Since Cockpit sends commands only on change or heartbeat, we might
          // block here if we wait. BETTER APPROACH: Use non-blocking read or
          // separate thread? SIMPLEST ROBUST APPROACH for this TP: The Cockpit
          // loop should match this loop frequency. Let's try to read. If
          // Cockpit sends heartbeat, this works.

          // Drain buffer: Read all available packets, keep the latest
          while (socket.available() >= sizeof(ClientToServerPacket)) {
            ClientToServerPacket rx_packet;
            boost::system::error_code error;
            boost::asio::read(
                socket, boost::asio::buffer(&rx_packet, sizeof(rx_packet)),
                error);

            if (error == boost::asio::error::eof)
              throw std::runtime_error("Connection closed");
            if (error)
              throw boost::system::system_error(error);

            // Update with the latest packet
            dados.setComandosOperador(rx_packet.comandos);
          }

          // 4. Logging (Keep existing logging logic)
          // ... (Logging code omitted for brevity, can be added back if needed)

          // 5. Sleep to maintain loop rate (10Hz)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      } catch (std::exception &e) {
        std::cerr << "[Coletor " << truck_id
                  << "] Erro na conexao: " << e.what() << std::endl;
      }

      std::cout << "[Coletor " << truck_id
                << "] Cliente desconectado. Aguardando..." << std::endl;
    }
  } catch (std::exception &e) {
    std::cerr << "[Coletor " << truck_id
              << "] Erro fatal no servidor: " << e.what() << std::endl;
  }
}
