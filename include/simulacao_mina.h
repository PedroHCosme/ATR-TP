#ifndef SIMULACAO_MINA_H
#define SIMULACAO_MINA_H

#include <vector>
#include <mutex>
#include <cmath>
#include <random> 
#include "dados.h"

struct CaminhaoFisico {
    int id;
    float i_posicao_x, i_posicao_y; // Posição em metros no mapa
    float i_angulo_x;               // Direção em graus
    float velocidade;               // Velocidade em m/s
    float o_aceleracao;              // Aceleração em m/s²
    float o_direcao;                 // Direção desejada em graus
    float i_temperatura;          // Temperatura do motor
    bool i_falha_eletrica;
    bool i_falha_hidraulica;
    int temperatuta_ambiente; // Temperatura ambiente

};

class SimulacaoMina {
    private:
    std::vector<CaminhaoFisico> frota; 
    const std::vector<std::vector<char>>& mapa;
    mutable std::mutex mtx_simulacao;
    float dt; // Delta tempo (passo da simulação)

    public:
    //construtor
    SimulacaoMina(const std::vector<std::vector<char>>& mapa_ref, int num_caminhoes);

    void atualizar_passo_tempo();


    // Define o que o caminhão quer fazer (Acelerar/Virar)
    void setComandoAtuador(int id_caminhao, int aceleracao, int direcao);

    // Obtém o estado real (usado pela TaskTratamentoSensores para gerar a leitura)
    CaminhaoFisico getEstadoReal(int id_caminhao);

private:
    // Métodos auxiliares internos
    void modelo_bicicleta(CaminhaoFisico& caminhao);
    void modelo_maquina_termica(CaminhaoFisico& caminhao);
    bool verificar_colisao(float x, float y, float angulo);
};






#endif // SIMULACAO_MINA_H