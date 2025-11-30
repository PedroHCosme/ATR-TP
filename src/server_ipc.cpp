#include "server_ipc.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

// JSON simples manual para evitar dependências externas pesadas
std::string build_json(const CaminhaoFisico &caminhao,
                       const EstadoVeiculo &estado,
                       const std::vector<std::vector<char>> &mapa,
                       bool send_map) {
  std::stringstream ss;
  ss << "{";

  // Estado do Caminhão
  ss << "\"truck\": {";
  ss << "\"x\": " << caminhao.i_posicao_x << ",";
  ss << "\"y\": " << caminhao.i_posicao_y << ",";
  ss << "\"angle\": " << caminhao.i_angulo_x << ",";
  ss << "\"velocity\": " << caminhao.velocidade << ",";
  ss << "\"temp\": " << caminhao.i_temperatura << ",";
  ss << "\"auto\": " << (estado.e_automatico ? "true" : "false") << ",";
  ss << "\"fault\": " << (estado.e_defeito ? "true" : "false");
  ss << "}";

  // Mapa (enviar apenas uma vez ou se solicitado seria melhor, mas aqui
  // simplificamos)
  if (send_map) {
    ss << ", \"map\": [";
    for (size_t y = 0; y < mapa.size(); ++y) {
      ss << "\"";
      for (char c : mapa[y])
        ss << c;
      ss << "\"";
      if (y < mapa.size() - 1)
        ss << ",";
    }
    ss << "]";
  }

  ss << "}";
  return ss.str();
}

ServerIPC::ServerIPC(GerenciadorDados &d, SimulacaoMina &s, EventosSistema &e,
                     const std::vector<std::vector<char>> &m)
    : server_fd(-1), client_fd(-1), running(false), dados(d), simulacao(s),
      eventos(e), mapa(m) {}

ServerIPC::~ServerIPC() { stop(); }

void ServerIPC::start() {
  running = true;
  net_thread = std::thread(&ServerIPC::loop, this);
}

void ServerIPC::stop() {
  running = false;
  if (client_fd >= 0)
    close(client_fd);
  if (server_fd >= 0)
    close(server_fd);
  if (net_thread.joinable())
    net_thread.join();
}

void ServerIPC::loop() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "[IPC] Erro ao criar socket" << std::endl;
    return;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(65432);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "[IPC] Erro no bind" << std::endl;
    return;
  }

  if (listen(server_fd, 1) < 0) {
    std::cerr << "[IPC] Erro no listen" << std::endl;
    return;
  }

  // std::cout << "[IPC] Aguardando conexao Python na porta 65432..." <<
  // std::endl;

  while (running) {
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (client_fd >= 0) {
      // std::cout << "[IPC] Cliente conectado!" << std::endl;
      handle_client();
      close(client_fd);
      client_fd = -1;
    }
  }
}

void ServerIPC::handle_client() {
  bool map_sent = false;

  while (running) {
    // 1. Envia Estado
    CaminhaoFisico real = simulacao.getEstadoReal(0);
    EstadoVeiculo estado = dados.getEstadoVeiculo();

    std::string json = build_json(real, estado, mapa, !map_sent);
    map_sent = true;

    uint32_t len = htonl(json.size());
    if (send(client_fd, &len, 4, MSG_NOSIGNAL) < 0)
      break;
    if (send(client_fd, json.c_str(), json.size(), MSG_NOSIGNAL) < 0)
      break;

    // 2. Recebe Comandos (Non-blocking check seria ideal, mas aqui vamos fazer
    // polling simples ou thread separada de leitura) Para simplificar, vamos
    // assumir que o Python manda comandos esporadicamente. Mas recv bloqueia.
    // Precisamos de select ou non-blocking.

    // Vamos usar um timeout curto no recv para não travar o loop de envio
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000; // 50ms
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
               sizeof tv);

    uint32_t cmd_len_net;
    int n = recv(client_fd, &cmd_len_net, 4, 0);
    if (n == 4) {
      uint32_t cmd_len = ntohl(cmd_len_net);
      std::vector<char> buffer(cmd_len + 1);
      n = recv(client_fd, buffer.data(), cmd_len, 0);
      if (n > 0) {
        buffer[n] = '\0';
        process_command(std::string(buffer.data()));
      }
    } else if (n == 0) {
      break; // Desconexão
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)); // 20Hz update rate
  }
}

void ServerIPC::process_command(const std::string &json) {
  // Parser manual muito simples
  ComandosOperador cmd = dados.getComandosOperador();

  if (json.find("\"mode\": \"auto\"") != std::string::npos) {
    cmd.c_automatico = true;
    cmd.c_man = false;
  } else if (json.find("\"mode\": \"manual\"") != std::string::npos) {
    cmd.c_automatico = false;
    cmd.c_man = true;
  } else if (json.find("\"type\": \"reset\"") != std::string::npos) {
    cmd.c_rearme = true;
    dados.setComandosOperador(cmd);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    cmd.c_rearme = false;
  } else if (json.find("\"type\": \"fault\"") != std::string::npos) {
    if (json.find("\"kind\": \"electric\"") != std::string::npos)
      eventos.sinalizar_falha(2);
    if (json.find("\"kind\": \"hydraulic\"") != std::string::npos)
      eventos.sinalizar_falha(3);
  } else if (json.find("\"type\": \"control\"") != std::string::npos) {
    // Reset manual controls
    cmd.c_acelerar = false;
    cmd.c_direita = false;
    cmd.c_esquerda = false;

    if (json.find("\"accel\": 1") != std::string::npos)
      cmd.c_acelerar = true;
    if (json.find("\"steer\": 1") != std::string::npos)
      cmd.c_direita = true;
    if (json.find("\"steer\": -1") != std::string::npos)
      cmd.c_esquerda = true;
  }

  dados.setComandosOperador(cmd);
}
