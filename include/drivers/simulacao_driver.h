#ifndef SIMULACAO_DRIVER_H
#define SIMULACAO_DRIVER_H

#include "../interfaces/i_sensor_driver.h"
#include "../simulacao_mina.h"

class SimulacaoDriver : public ISensorDriver {
    private:
    //referencia para a simulação física
    SimulacaoMina& simulacao;
    public:
    //construtor
    SimulacaoDriver(SimulacaoMina& sim) : simulacao(sim) {}

    //implementação do método da interface
    CaminhaoFisico readSensorData(int id_caminhao) override {
        return simulacao.getEstadoReal(id_caminhao);
    }
};

#endif // SIMULACAO_DRIVER_H
