#include "simulacao_mina.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

// Constantes do Mapa e Veículo (Movidas para cá)
const float CELL_SIZE = 10.0f;
const float TRUCK_WIDTH = 4.0f;
const float TRUCK_LENGTH = 6.0f;

SimulacaoMina::SimulacaoMina(const std::vector<std::vector<char>> &mapa_ref,
                             int num_caminhoes)
    : mapa(mapa_ref), dt(0.1f) {

  // Inicializa a frota
  for (int i = 0; i < num_caminhoes; ++i) {
    CaminhaoFisico c;
    c.id = i;

    // Encontrar ponto de partida 'A' no mapa
    float start_x = 25.0f;
    float start_y = 25.0f;
    for (size_t y = 0; y < mapa.size(); ++y) {
      for (size_t x = 0; x < mapa[y].size(); ++x) {
        if (mapa[y][x] == 'A') {
          start_x = x * CELL_SIZE + CELL_SIZE / 2.0f;
          start_y = y * CELL_SIZE + CELL_SIZE / 2.0f;
        }
      }
    }

    c.i_posicao_x = start_x;
    c.i_posicao_y = start_y;
    c.i_angulo_x = 0.0f;
    c.velocidade = 0.0f;
    c.o_aceleracao = 0.0f;
    c.o_direcao = 0.0f;   // Direção inicial
    c.i_temperatura = 85; // Temperatura inicial
    c.temperatura_ambiente = 12;
    c.i_falha_eletrica = false;
    c.i_falha_hidraulica = false;
    c.i_lidar_distancia = 100.0f; // Distância inicial segura

    // std::cout << "[DEBUG-INIT] Caminhao " << i << " criado. Pos: (" <<
    // start_x
    //<< "," << start_y << ") Vel: " << c.velocidade
    //<< " Acel: " << c.o_aceleracao << std::endl;

    frota.push_back(c);
  }
}

void SimulacaoMina::atualizar_passo_tempo() {
  std::lock_guard<std::mutex> lock(mtx_simulacao);
  for (auto &caminhao : frota) {
    modelo_bicicleta(caminhao);
    modelo_maquina_termica(caminhao);
  }
}

void SimulacaoMina::modelo_bicicleta(CaminhaoFisico &caminhao) {
  // Debug antes de atualizar
  // std::cout << "[DEBUG-PRE-UPD] Vel: " << caminhao.velocidade << " Acel: " <<
  // caminhao.o_aceleracao << std::endl;

  // 1. Atualiza velocidade baseado na aceleração (conversão de % para
  // aceleração real) o_aceleracao vai de -100 a 100%, convertemos para m/s²
  float aceleracao_real = caminhao.o_aceleracao * 0.2f; // Máximo de 20 m/s²
  caminhao.velocidade += aceleracao_real * dt;

  // Limita velocidade (não pode ser negativa em modelo simplificado)
  if (caminhao.velocidade < 0.0f)
    caminhao.velocidade = 0.0f;
  if (caminhao.velocidade > 25.0f)
    caminhao.velocidade = 25.0f; // Limite máximo ~90 km/h

  // 2. Atualiza direção baseado no comando o_direcao
  // No modo manual, o_direcao já é o ângulo absoluto
  // Aplicamos uma taxa de giro gradual para suavizar
  float diff_angulo = caminhao.o_direcao - caminhao.i_angulo_x;
  while (diff_angulo > 180.0f)
    diff_angulo -= 360.0f;
  while (diff_angulo < -180.0f)
    diff_angulo += 360.0f;

  float taxa_giro = 50.0f; // graus por segundo
  float ajuste =
      std::max(-taxa_giro * dt, std::min(taxa_giro * dt, diff_angulo));
  caminhao.i_angulo_x += ajuste;

  if (caminhao.i_angulo_x >= 360.0f)
    caminhao.i_angulo_x -= 360.0f;
  if (caminhao.i_angulo_x < 0.0f)
    caminhao.i_angulo_x += 360.0f;

  // 3. Calcula nova posição baseada na velocidade e direção
  float rad_ang = caminhao.i_angulo_x * M_PI / 180.0f;
  float next_x =
      caminhao.i_posicao_x + caminhao.velocidade * std::cos(rad_ang) * dt;
  float next_y =
      caminhao.i_posicao_y + caminhao.velocidade * std::sin(rad_ang) * dt;

  if (verificar_colisao(next_x, next_y, caminhao.i_angulo_x)) {
    // Colisão detectada: Para o caminhão e inverte direção
    // std::cout << "!!! COLISÃO DETECTADA (Simulação) !!! Parando e virando..."
    //           << std::endl;
    caminhao.velocidade = 0.0f; // Para imediatamente
    caminhao.i_angulo_x += 180.0f + (std::rand() % 60 - 30);
    if (caminhao.i_angulo_x >= 360.0f)
      caminhao.i_angulo_x -= 360.0f;
    if (caminhao.i_angulo_x < 0.0f)
      caminhao.i_angulo_x += 360.0f;
  } else {
    // Caminho livre - atualiza posição
    caminhao.i_posicao_x = next_x;
    caminhao.i_posicao_y = next_y;
  }

  // Debug: Print de velocidade e aceleração
  // std::cout << "[DEBUG-FISICA] Vel: " << caminhao.velocidade
  //           << " | Acel: " << caminhao.o_aceleracao
  //           << "% | Ang: " << caminhao.i_angulo_x << "°" << std::endl;

  // --- SIMULAÇÃO DE OBSTÁCULO DINÂMICO (LIDAR REAL) ---
  caminhao.i_lidar_distancia = calcular_lidar(caminhao);
}

float SimulacaoMina::calcular_lidar(const CaminhaoFisico &caminhao) {
  float x = caminhao.i_posicao_x;
  float y = caminhao.i_posicao_y;
  float ang_rad = caminhao.i_angulo_x * M_PI / 180.0f;
  float dx = std::cos(ang_rad);
  float dy = std::sin(ang_rad);

  float max_dist = 100.0f;
  float step_size = 1.0f; // 1 meter precision (was CELL_SIZE/2 = 5m)
  float dist = 0.0f;

  while (dist < max_dist) {
    dist += step_size;
    float check_x = x + dx * dist;
    float check_y = y + dy * dist;

    int grid_x = static_cast<int>(check_x / CELL_SIZE);
    int grid_y = static_cast<int>(check_y / CELL_SIZE);

    // Check bounds
    if (grid_y < 0 || grid_y >= (int)mapa.size() || grid_x < 0 ||
        grid_x >= (int)mapa[0].size()) {
      return dist; // Hit world edge
    }

    // Check wall
    if (mapa[grid_y][grid_x] == '1') {
      return dist; // Hit wall
    }
  }
  return max_dist;
}

void SimulacaoMina::modelo_maquina_termica(CaminhaoFisico &caminhao) {
  // Modelo termodinâmico simples
  // Aquece proporcionalmente à velocidade, esfria em direção à temperatura
  // ambiente
  float heat_gen = std::abs(caminhao.velocidade) * 0.5f;
  float heat_loss =
      0.1f * (caminhao.i_temperatura - caminhao.temperatura_ambiente);

  caminhao.i_temperatura += (heat_gen - heat_loss) * dt;

  // Debug: Print de temperatura a cada atualização
  // std::cout << "[DEBUG-FISICA] Temp: " << temp_anterior << " -> "
  //           << caminhao.i_temperatura << " | Vel: " << caminhao.velocidade
  //           << " | Heat+" << heat_gen << " -" << heat_loss << std::endl;
}

bool SimulacaoMina::verificar_colisao(float x, float y, float angulo) {
  float rad = angulo * M_PI / 180.0f;
  float cos_a = std::cos(rad);
  float sin_a = std::sin(rad);

  float w2 = TRUCK_WIDTH / 2.0f;
  float l2 = TRUCK_LENGTH / 2.0f;

  float corners_x[] = {l2, l2, -l2, -l2};
  float corners_y[] = {-w2, w2, -w2, w2};

  int map_height = mapa.size();
  int map_width = mapa[0].size();

  for (int i = 0; i < 4; i++) {
    float global_x = x + (corners_x[i] * cos_a - corners_y[i] * sin_a);
    float global_y = y + (corners_x[i] * sin_a + corners_y[i] * cos_a);

    int grid_x = static_cast<int>(global_x / CELL_SIZE);
    int grid_y = static_cast<int>(global_y / CELL_SIZE);

    if (grid_x < 0 || grid_x >= map_width || grid_y < 0 ||
        grid_y >= map_height) {
      // std::cout << "[DEBUG-COLISAO] Fora do mapa: " << grid_x << "," <<
      // grid_y
      //<< std::endl;
      return true;
    }
    if (mapa[grid_y][grid_x] == '1') {
      // std::cout << "[DEBUG-COLISAO] Parede em: " << grid_x << "," << grid_y
      //           << " (Global: " << global_x << "," << global_y << ")"
      //           << std::endl;
      return true;
    }
  }
  return false;
}

CaminhaoFisico SimulacaoMina::getEstadoReal(int id_caminhao) {
  std::lock_guard<std::mutex> lock(mtx_simulacao);
  if (id_caminhao >= 0 && id_caminhao < (int)frota.size()) {
    return frota[id_caminhao];
  }
  return CaminhaoFisico();
}

void SimulacaoMina::setComandoAtuador(int id_caminhao, int aceleracao,
                                      int direcao) {
  std::lock_guard<std::mutex> lock(mtx_simulacao);
  if (id_caminhao >= 0 && id_caminhao < (int)frota.size()) {
    // std::cout << "[DEBUG-ATUADOR] Recebido: Acel=" << aceleracao
    //           << "% | Dir=" << direcao << "°" << std::endl;
    frota[id_caminhao].o_aceleracao = aceleracao;
    frota[id_caminhao].o_direcao = direcao;
  }
}
