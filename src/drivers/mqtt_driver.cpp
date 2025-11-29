#include "drivers/mqtt_driver.h"
#include <iostream>
#include <cstring>

MqttDriver::MqttDriver(const std::string& broker_ip, int port) 
    : broker_ip(broker_ip), port(port), conectado(false) {
    
    // Inicializa dados com zeros
    ultimos_dados = {0, 0, 0, 0, 0, 0};

    mosquitto_lib_init();
    mosq = mosquitto_new("ATR_Control_System", true, this);

    if (!mosq) {
        std::cerr << "[MqttDriver] Erro ao criar instancia mosquitto" << std::endl;
        return;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    int rc = mosquitto_connect(mosq, broker_ip.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "[MqttDriver] Erro ao conectar ao broker: " << mosquitto_strerror(rc) << std::endl;
    } else {
        mosquitto_loop_start(mosq);
    }
}

MqttDriver::~MqttDriver() {
    if (mosq) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
}

void MqttDriver::on_connect(struct mosquitto* mosq, void* obj, int rc) {
    MqttDriver* driver = static_cast<MqttDriver*>(obj);
    if (rc == 0) {
        std::cout << "[MqttDriver] Conectado ao broker!" << std::endl;
        driver->conectado = true;
        mosquitto_subscribe(mosq, NULL, "caminhao/sensores", 0);
    } else {
        std::cerr << "[MqttDriver] Falha na conexao: " << rc << std::endl;
    }
}

void MqttDriver::on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
    MqttDriver* driver = static_cast<MqttDriver*>(obj);
    std::string topic(static_cast<char*>(msg->topic));
    std::string payload(static_cast<char*>(msg->payload), msg->payloadlen);

    if (topic == "caminhao/sensores") {
        driver->handle_sensor_message(payload);
    }
}

void MqttDriver::handle_sensor_message(const std::string& payload) {
    try {
        auto j = json::parse(payload);
        std::lock_guard<std::mutex> lock(dados_mtx);
        
        if (j.contains("id")) ultimos_dados.id = j["id"];
        if (j.contains("x")) ultimos_dados.i_posicao_x = j["x"];
        if (j.contains("y")) ultimos_dados.i_posicao_y = j["y"];
        if (j.contains("angle")) ultimos_dados.i_angulo_x = j["angle"];
        if (j.contains("vel")) ultimos_dados.i_velocidade = j["vel"];
        if (j.contains("temp")) ultimos_dados.i_temperatura = j["temp"];
        
    } catch (const std::exception& e) {
        std::cerr << "[MqttDriver] Erro ao parsear JSON: " << e.what() << std::endl;
    }
}

CaminhaoFisico MqttDriver::readSensorData(int id) {
    std::lock_guard<std::mutex> lock(dados_mtx);
    // Converte DadosSensores (struct interna) para CaminhaoFisico (interface)
    // Na verdade, DadosSensores e CaminhaoFisico são parecidos, mas CaminhaoFisico é o estado real.
    // O driver deve retornar o que leu.
    CaminhaoFisico c;
    c.i_posicao_x = ultimos_dados.i_posicao_x;
    c.i_posicao_y = ultimos_dados.i_posicao_y;
    c.i_angulo_x = ultimos_dados.i_angulo_x;
    c.velocidade = ultimos_dados.i_velocidade;
    c.i_temperatura = ultimos_dados.i_temperatura;
    return c;
}

DadosSensores MqttDriver::lerDados(int id) {
    std::lock_guard<std::mutex> lock(dados_mtx);
    return ultimos_dados;
}

void MqttDriver::setAtuadores(int aceleracao, int direcao) {
    if (!conectado) return;

    json j;
    j["acel"] = aceleracao;
    j["dir"] = direcao;
    
    std::string payload = j.dump();
    mosquitto_publish(mosq, NULL, "caminhao/atuadores", payload.length(), payload.c_str(), 0, false);
}
