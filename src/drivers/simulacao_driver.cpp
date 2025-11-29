#include "drivers/simulacao_driver.h"

// Construtor já está definido inline no header, não precisa repetir aqui

CaminhaoFisico SimulacaoDriver::readSensorData(int id_caminhao) {
    return simulacao.getEstadoReal(id_caminhao);
}

void SimulacaoDriver::setAtuadores(int aceleracao, int direcao) {
    // Repassa para a simulação física (Assumindo ID 0 por enquanto)
    simulacao.setComandoAtuador(0, aceleracao, direcao);
}