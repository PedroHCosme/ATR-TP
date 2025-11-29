#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include "simulacao_mina.h"
#include "mine_generator.h"
#include "server_ipc.h" // Mantemos o IPC para a interface visual (Pygame)
#include "gerenciador_dados.h" // Necessário para o ServerIPC
#include "eventos_sistema.h"   // Necessário para o ServerIPC

using json = nlohmann::json;

// Variáveis globais para o simulador headless
struct mosquitto* mosq = nullptr;
std::atomic<int> cmd_acel(0);
std::atomic<int> cmd_dir(0);

void on_connect(struct mosquitto* m, void* obj, int rc) {
    if (rc == 0) {
        std::cout << "[Simulador] Conectado ao broker MQTT!" << std::endl;
        mosquitto_subscribe(m, NULL, "caminhao/atuadores", 0);
    } else {
        std::cerr << "[Simulador] Falha na conexao MQTT: " << rc << std::endl;
    }
}

void on_message(struct mosquitto* m, void* obj, const struct mosquitto_message* msg) {
    std::string topic(static_cast<char*>(msg->topic));
    std::string payload(static_cast<char*>(msg->payload), msg->payloadlen);

    if (topic == "caminhao/atuadores") {
        try {
            auto j = json::parse(payload);
            if (j.contains("acel")) cmd_acel = j["acel"];
            if (j.contains("dir")) cmd_dir = j["dir"];
        } catch (...) {
            std::cerr << "[Simulador] Erro JSON atuadores" << std::endl;
        }
    }
}

int main() {
    std::cout << "--- INICIANDO SIMULADOR HEADLESS (FISICA + MQTT) ---" << std::endl;

    // 1. Setup MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new("ATR_Physics_Engine", true, nullptr);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    
    if (mosquitto_connect(mosq, "localhost", 1883, 60) != MOSQ_ERR_SUCCESS) {
        std::cerr << "[Simulador] Nao foi possivel conectar ao Mosquitto local." << std::endl;
        // Continua mesmo sem MQTT para teste local, ou aborta? Vamos continuar.
    }
    mosquitto_loop_start(mosq);

    // 2. Setup Física
    MineGenerator mineGen(61, 61);
    mineGen.generate();
    SimulacaoMina simulacao(mineGen.getMinefield(), 1);

    // 3. Setup Interface Visual (Pygame) - Mantemos para ver o que acontece
    // Precisamos de objetos dummy para o ServerIPC, pois ele espera GerenciadorDados
    GerenciadorDados dadosDummy; 
    EventosSistema eventosDummy;
    ServerIPC visualServer(dadosDummy, simulacao, eventosDummy, mineGen.getMinefield());
    visualServer.start();

    std::cout << "Simulacao rodando..." << std::endl;

    // Loop de Física (10Hz)
    while (true) {
        auto start_time = std::chrono::steady_clock::now();

        // 1. Aplica comandos recebidos via MQTT
        simulacao.setComandoAtuador(0, cmd_acel, cmd_dir);

        // 2. Passo de tempo
        simulacao.atualizar_passo_tempo();

        // 3. Publica estado via MQTT
        CaminhaoFisico estado = simulacao.getEstadoReal(0);
        json j;
        j["id"] = 0;
        j["x"] = (int)estado.i_posicao_x;
        j["y"] = (int)estado.i_posicao_y;
        j["angle"] = (int)estado.i_angulo_x;
        j["vel"] = (int)estado.velocidade;
        j["temp"] = (int)estado.i_temperatura;

        std::string payload = j.dump();
        mosquitto_publish(mosq, NULL, "caminhao/sensores", payload.length(), payload.c_str(), 0, false);

        // 4. Atualiza dados para o Visualizador (Pygame)
        // O ServerIPC lê direto da 'simulacao', então ok.
        // Mas ele lê 'dados.getEstadoVeiculo()' para saber se está em auto/manual.
        // Como separamos, o simulador não sabe o modo de operação do controlador.
        // Vamos deixar como está, o visualizador vai mostrar o caminhão se movendo, 
        // mas o status "AUTO/MANUAL" pode ficar incorreto no Pygame se não sincronizarmos.
        // Para este passo, focamos na física.

        // Mantém 10Hz
        std::this_thread::sleep_until(start_time + std::chrono::milliseconds(100));
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    visualServer.stop();

    return 0;
}
