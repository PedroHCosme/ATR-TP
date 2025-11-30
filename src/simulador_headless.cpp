#include "eventos_sistema.h"   // Necessário para o ServerIPC
#include "gerenciador_dados.h" // Necessário para o ServerIPC
#include "mine_generator.h"
#include "server_ipc.h" // Mantemos o IPC para a interface visual (Pygame)
#include "simulacao_mina.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

// Variáveis globais para o simulador headless
struct mosquitto *mosq = nullptr;
std::atomic<int> global_cmd_acel[3];
std::atomic<int> global_cmd_dir[3];

// Objetos globais para comunicação com ServerIPC
GerenciadorDados dadosDummy;
EventosSistema eventosDummy;

void on_connect(struct mosquitto *m, void *obj, int rc) {
  if (rc == 0) {
    // std::cout << "[Simulador] Conectado ao broker MQTT!" << std::endl;
    mosquitto_subscribe(m, NULL, "caminhao/atuadores", 0);
    mosquitto_subscribe(m, NULL, "caminhao/estado_sistema", 0);
  } else {
    std::cerr << "[Simulador] Falha na conexao MQTT: " << rc << std::endl;
  }
}

void on_message(struct mosquitto *m, void *obj,
                const struct mosquitto_message *msg) {
  std::string topic(static_cast<char *>(msg->topic));
  std::string payload(static_cast<char *>(msg->payload), msg->payloadlen);

  if (topic == "caminhao/atuadores") {
    try {
      auto j = json::parse(payload);
      int id = 0;
      if (j.contains("id"))
        id = j["id"];

      int acel = 0;
      int dir = 0;
      if (j.contains("acel"))
        acel = j["acel"];
      if (j.contains("dir"))
        dir = j["dir"];

      // Apply directly to simulation (thread-safe via mutex in SimulacaoMina)
      // We need access to 'simulacao' object here.
      // But 'simulacao' is in main(). We need to make it global or pass it.
      // It is currently local in main.
      // Let's make 'simulacao' global or use the 'cmd_acel' arrays.

      // For now, let's use global arrays for commands.
      if (id >= 0 && id < 3) {
        global_cmd_acel[id] = acel;
        global_cmd_dir[id] = dir;
      }
    } catch (...) {
      std::cerr << "[Simulador] Erro JSON atuadores" << std::endl;
    }
  } else if (topic == "caminhao/estado_sistema") {
    try {
      auto j = json::parse(payload);

      if (j.contains("manual")) {
        bool manual = j["manual"];
        EstadoVeiculo e = dadosDummy.getEstadoVeiculo();
        e.e_automatico = !manual;
        dadosDummy.setEstadoVeiculo(e);
      }

      if (j.contains("fault")) {
        bool fault = j["fault"];
        if (fault) {
          eventosDummy.sinalizar_falha(99); // Código genérico de falha externa
        } else {
          eventosDummy.resetar_falhas();
        }
      }

    } catch (...) {
      std::cerr << "[Simulador] Erro JSON estado sistema" << std::endl;
    }
  }
}

int main() {
  // std::cout << "--- INICIANDO SIMULADOR HEADLESS (FISICA + MQTT) ---"
  // << std::endl;

  // 1. Setup MQTT
  mosquitto_lib_init();
  mosq = mosquitto_new("ATR_Physics_Engine", true, nullptr);
  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_message_callback_set(mosq, on_message);

  int rc = mosquitto_connect(mosq, "127.0.0.1", 1883, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
    std::cerr
        << "[Simulador] Nao foi possivel conectar ao Mosquitto local. Erro: "
        << rc << " (" << mosquitto_strerror(rc) << ")" << std::endl;
    // Continua mesmo sem MQTT para teste local, ou aborta? Vamos continuar.
  }
  mosquitto_loop_start(mosq);

  // 2. Setup Física
  MineGenerator mineGen(61, 61);
  mineGen.generate();
  SimulacaoMina simulacao(mineGen.getMinefield(), 3);

  // Publicar Mapa (Retained)
  json j_map;
  j_map["map"] = mineGen.getMinefield();
  j_map["width"] = 61; // Hardcoded for now based on mineGen(61, 61)
  j_map["height"] = 61;
  std::string map_payload = j_map.dump();
  mosquitto_publish(mosq, NULL, "caminhao/mapa", map_payload.length(),
                    map_payload.c_str(), 0, true);
  // std::cout << "[Simulador] Mapa publicado (retained)." << std::endl;

  // 3. Setup Interface Visual (Pygame) - Mantemos para ver o que acontece
  // Precisamos de objetos dummy para o ServerIPC, pois ele espera
  // GerenciadorDados
  // (dadosDummy e eventosDummy agora são globais)
  ServerIPC visualServer(dadosDummy, simulacao, eventosDummy,
                         mineGen.getMinefield());
  visualServer.start();

  // std::cout << "Simulacao rodando..." << std::endl;

  // Loop de Física (10Hz)
  while (true) {
    auto start_time = std::chrono::steady_clock::now();

    // 1. Aplica comandos recebidos via MQTT
    // std::cout << "[Simulador] Loop tick" << std::endl;
    // 1. Aplica comandos recebidos via MQTT
    // std::cout << "[Simulador] Loop tick" << std::endl;
    for (int i = 0; i < 3; ++i) {
      simulacao.setComandoAtuador(i, global_cmd_acel[i], global_cmd_dir[i]);
    }

    // 2. Passo de tempo
    // std::cout << "[Simulador] Step 2" << std::endl;
    simulacao.atualizar_passo_tempo();

    // 3. Publica estado via MQTT
    // std::cout << "[Simulador] Step 3" << std::endl;
    // 3. Publica estado via MQTT
    // std::cout << "[Simulador] Step 3" << std::endl;
    for (int i = 0; i < 3; ++i) {
      CaminhaoFisico estado = simulacao.getEstadoReal(i);
      json j;
      j["id"] = i;
      j["x"] = estado.i_posicao_x;
      j["y"] = estado.i_posicao_y;
      j["angle"] = estado.i_angulo_x;
      j["vel"] = estado.velocidade;
      j["temp"] = estado.i_temperatura;
      j["lidar"] = estado.i_lidar_distancia;

      std::string payload = j.dump();
      mosquitto_publish(mosq, NULL, "caminhao/sensores", payload.length(),
                        payload.c_str(), 0, false);
    }

    // 4. Atualiza dados para o Visualizador (Pygame)
    // O ServerIPC lê direto da 'simulacao', então ok.
    // Mas ele lê 'dados.getEstadoVeiculo()' para saber se está em auto/manual.
    // Como separamos, o simulador não sabe o modo de operação do controlador.
    // Vamos deixar como está, o visualizador vai mostrar o caminhão se movendo,
    // mas o status "AUTO/MANUAL" pode ficar incorreto no Pygame se não
    // sincronizarmos. Para este passo, focamos na física.

    // Mantém 10Hz
    std::this_thread::sleep_until(start_time + std::chrono::milliseconds(100));
  }

  mosquitto_loop_stop(mosq, true);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  visualServer.stop();

  return 0;
}
