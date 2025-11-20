#include "simulacao_mina.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

// Constantes do Mapa e Veículo (Movidas para cá)
const float CELL_SIZE = 10.0f; 
const float TRUCK_WIDTH = 4.0f;
const float TRUCK_LENGTH = 6.0f;

SimulacaoMina::SimulacaoMina(const std::vector<std::vector<char>>& mapa_ref, int num_caminhoes)
    : mapa(mapa_ref), dt(0.1f) {
    
    // Inicializa a frota
    for(int i=0; i<num_caminhoes; ++i) {
        CaminhaoFisico c;
        c.id = i;
        
        // Encontrar ponto de partida 'A' no mapa
        float start_x = 15.0f;
        float start_y = 15.0f;
        for(size_t y=0; y<mapa.size(); ++y) {
            for(size_t x=0; x<mapa[y].size(); ++x) {
                if(mapa[y][x] == 'A') {
                    start_x = x * CELL_SIZE + CELL_SIZE/2.0f;
                    start_y = y * CELL_SIZE + CELL_SIZE/2.0f;
                }
            }
        }
        
        c.i_posicao_x = start_x;
        c.i_posicao_y = start_y;
        c.i_angulo_x = 0.0f;
        c.velocidade = 20.0f; // Velocidade inicial
        c.i_temperatura = 85; // Temperatura inicial
        c.temperatuta_ambiente = 25;
        c.i_falha_eletrica = false;
        c.i_falha_hidraulica = false;
        
        frota.push_back(c);
    }
}

void SimulacaoMina::atualizar_passo_tempo() {
    std::lock_guard<std::mutex> lock(mtx_simulacao);
    for(auto& caminhao : frota) {
        modelo_bicicleta(caminhao);
        modelo_maquina_termica(caminhao);
    }
}

void SimulacaoMina::modelo_bicicleta(CaminhaoFisico& caminhao) {
    float rad_ang = caminhao.i_angulo_x * M_PI / 180.0f;
    float next_x = caminhao.i_posicao_x + caminhao.velocidade * std::cos(rad_ang) * dt;
    float next_y = caminhao.i_posicao_y + caminhao.velocidade * std::sin(rad_ang) * dt;

    if (verificar_colisao(next_x, next_y, caminhao.i_angulo_x)) {
        // Colisão detectada: Inverte direção e adiciona variação
        std::cout << "!!! COLISÃO DETECTADA (Simulação) !!! Virando..." << std::endl;
        caminhao.i_angulo_x += 180.0f + (std::rand() % 60 - 30);
        if (caminhao.i_angulo_x >= 360.0f) caminhao.i_angulo_x -= 360.0f;
        if (caminhao.i_angulo_x < 0.0f) caminhao.i_angulo_x += 360.0f;
    } else {
        // Caminho livre
        caminhao.i_posicao_x = next_x;
        caminhao.i_posicao_y = next_y;
        
        // Variação aleatória para exploração
        if (std::rand() % 10 == 0) {
             caminhao.i_angulo_x += (std::rand() % 10 - 5);
        }
    }
}

void SimulacaoMina::modelo_maquina_termica(CaminhaoFisico& caminhao) {
    // Modelo termodinâmico simples
    // Aquece proporcionalmente à velocidade, esfria em direção à temperatura ambiente
    float heat_gen = std::abs(caminhao.velocidade) * 0.5f; 
    float heat_loss = 0.1f * (caminhao.i_temperatura - caminhao.temperatuta_ambiente);
    
    caminhao.i_temperatura += (heat_gen - heat_loss) * dt;
}

bool SimulacaoMina::verificar_colisao(float x, float y, float angulo) {
    float rad = angulo * M_PI / 180.0f;
    float cos_a = std::cos(rad);
    float sin_a = std::sin(rad);

    float w2 = TRUCK_WIDTH / 2.0f;
    float l2 = TRUCK_LENGTH / 2.0f;

    float corners_x[] = { l2,  l2, -l2, -l2};
    float corners_y[] = {-w2,  w2, -w2,  w2};

    int map_height = mapa.size();
    int map_width = mapa[0].size();

    for (int i = 0; i < 4; i++) {
        float global_x = x + (corners_x[i] * cos_a - corners_y[i] * sin_a);
        float global_y = y + (corners_x[i] * sin_a + corners_y[i] * cos_a);

        int grid_x = static_cast<int>(global_x / CELL_SIZE);
        int grid_y = static_cast<int>(global_y / CELL_SIZE);

        if (grid_x < 0 || grid_x >= map_width || grid_y < 0 || grid_y >= map_height) return true;
        if (mapa[grid_y][grid_x] == '1') return true;
    }
    return false;
}

CaminhaoFisico SimulacaoMina::getEstadoReal(int id_caminhao) {
    std::lock_guard<std::mutex> lock(mtx_simulacao);
    if(id_caminhao >= 0 && id_caminhao < (int)frota.size()) {
        return frota[id_caminhao];
    }
    return CaminhaoFisico();
}

void SimulacaoMina::setComandoAtuador(int id_caminhao, int aceleracao, int direcao) {
    std::lock_guard<std::mutex> lock(mtx_simulacao);
    if(id_caminhao >= 0 && id_caminhao < (int)frota.size()) {
        frota[id_caminhao].o_aceleracao = aceleracao;
        frota[id_caminhao].o_direcao = direcao;
    }
}

