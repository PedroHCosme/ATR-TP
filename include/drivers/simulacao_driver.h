#ifndef SIMULACAO_DRIVER_H
#define SIMULACAO_DRIVER_H

#include "../interfaces/i_sensor_driver.h"
#include "../interfaces/i_veiculo_driver.h"
#include "../simulacao_mina.h"

class SimulacaoDriver : public ISensorDriver, public IVeiculoDriver {
    private:
    //referencia para a simulação física
    SimulacaoMina& simulacao;
    public:
    //construtor
    SimulacaoDriver(SimulacaoMina& sim) : simulacao(sim) {}

    //declaração do método da interface do sensor
    CaminhaoFisico readSensorData(int id_caminhao) override;

    //declaração do método da interface do veículo
    void setAtuadores(int aceleracao, int direcao) override;
};

#endif // SIMULACAO_DRIVER_H
