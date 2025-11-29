#ifndef MQTT_DRIVER_H
#define MQTT_DRIVER_H

#include <string>
#include <mutex>
#include <atomic>
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include "interfaces/i_sensor_driver.h"
#include "interfaces/i_veiculo_driver.h"
#include "dados.h"

using json = nlohmann::json;

class MqttDriver : public ISensorDriver, public IVeiculoDriver {
private:
    struct mosquitto* mosq;
    std::string broker_ip;
    int port;
    
    DadosSensores ultimos_dados;
    std::mutex dados_mtx;
    std::atomic<bool> conectado;

public:
    MqttDriver(const std::string& broker_ip, int port = 1883);
    ~MqttDriver();

    // ISensorDriver
    CaminhaoFisico readSensorData(int id) override;
    DadosSensores lerDados(int id); // Mantemos para compatibilidade se necessário, mas a interface pede readSensorData

    // IVeiculoDriver
    void setAtuadores(int aceleracao, int direcao) override;

    // Callbacks
    static void on_connect(struct mosquitto* mosq, void* obj, int rc);
    static void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);

private:
    void handle_sensor_message(const std::string& payload);
};

#endif // MQTT_DRIVER_H
