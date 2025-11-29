#include "drivers/simulacao_driver.h"

SimulacaoDriver::SimulacaoDriver(SimulacaoMina& sim) : simulacao(sim) {}

CaminhaoFisico SimulacaoDriver::readSensorData(int id_caminhao) {
    return simulacao.getEstadoReal(id_caminhao);
}

void SimulacaoDriver::setAtuadores(int aceleracao, int direcao) {
    // Repassa para a simulação física (Assumindo ID 0 por enquanto)
    simulacao.setComandoAtuador(0, aceleracao, direcao);
}