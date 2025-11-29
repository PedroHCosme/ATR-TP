#include "drivers/simulacao_driver.h"

SimulacaoDriver::SimulacaoDriver(SimulacaoMina& sim) : simulacao(sim) {}

CaminhaoFisico SimulacaoDriver::readSensorData(int id_caminhao) {
    return simulacao.getEstadoReal(id_caminhao);
}
