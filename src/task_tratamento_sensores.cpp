#include "task_tratamento_sensores.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <vector>

// Constantes do filtro
const int N = 10; // Número de períodos para suavização
const float K = 2.0f / (1.0f + N); // Fator de suavização

// Constantes do Mapa e Veículo
const float CELL_SIZE = 10.0f; // Tamanho de cada célula do grid em metros 
const float TRUCK_WIDTH = 4.0f;  // Largura do caminhão
const float TRUCK_LENGTH = 6.0f; // Comprimento do caminhão

/**
 * @brief Calcula o próximo valor da Média Móvel Exponencial (EMA).
 * 
 * @param valor_atual O valor bruto lido do sensor agora.
 * @param media_anterior O valor da EMA calculado no passo anterior.
 * @return float O novo valor da EMA.
 */
float calcular_media_movel_exponencial(float valor_atual, float media_anterior) {
    return (valor_atual - media_anterior) * K + media_anterior;
}

/**
 * @brief Verifica colisão do caminhão com as paredes do mapa.
 * 
 * Calcula a bounding box do caminhão baseada na posição e ângulo,
 * e verifica se algum dos 4 cantos está dentro de uma célula de parede ('1').
 * 
 * @param x Posição X do centro do caminhão.
 * @param y Posição Y do centro do caminhão.
 * @param angulo Ângulo do caminhão em graus.
 * @param mapa Referência para o grid do mapa.
 * @return true se houver colisão, false caso contrário.
 */
bool verificar_colisao(float x, float y, float angulo, const std::vector<std::vector<char>>& mapa) {
    float rad = angulo * M_PI / 180.0f;
    float cos_a = std::cos(rad);
    float sin_a = std::sin(rad);

    // Metade das dimensões
    float w2 = TRUCK_WIDTH / 2.0f;
    float l2 = TRUCK_LENGTH / 2.0f;

    // Coordenadas relativas dos 4 cantos (Frente-Esq, Frente-Dir, Tras-Esq, Tras-Dir)
    float corners_x[] = { l2,  l2, -l2, -l2};
    float corners_y[] = {-w2,  w2, -w2,  w2};

    int map_height = mapa.size();
    int map_width = mapa[0].size();

    for (int i = 0; i < 4; i++) {
        // Rotaciona e translada cada ponto para o mundo global
        float global_x = x + (corners_x[i] * cos_a - corners_y[i] * sin_a);
        float global_y = y + (corners_x[i] * sin_a + corners_y[i] * cos_a);

        // Converte coordenada global (metros) para índice do grid
        int grid_x = static_cast<int>(global_x / CELL_SIZE);
        int grid_y = static_cast<int>(global_y / CELL_SIZE);

        // Verifica limites do mapa
        if (grid_x < 0 || grid_x >= map_width || grid_y < 0 || grid_y >= map_height) {
            return true; // Saiu do mapa
        }

        // Verifica se é parede
        if (mapa[grid_y][grid_x] == '1') {
            return true; // Colidiu com parede
        }
    }
    return false;
}

/**
 * @brief Tarefa responsável pela leitura e tratamento dos dados dos sensores.
 * 
 * Esta função simula a leitura de sensores de um caminhão em movimento dentro do mapa gerado.
 * Ela verifica colisões e implementa uma lógica simples de "bate-e-volta".
 * 
 * @param gerenciadorDados Referência para o gerenciador de dados compartilhado.
 * @param mapa Referência para o mapa da mina.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados, const std::vector<std::vector<char>>& mapa) {
    std::default_random_engine generator;
    
    // Configuração dos geradores de ruído
    std::normal_distribution<float> noise_pos(0.0, 1.0); 
    std::normal_distribution<float> noise_ang(0.0, 2.0); 
    std::uniform_real_distribution<float> temp_dist(80, 100);

    // Encontrar ponto de partida 'A' no mapa
    float start_x = 15.0f; // Default fallback
    float start_y = 15.0f;
    for(size_t y=0; y<mapa.size(); ++y) {
        for(size_t x=0; x<mapa[y].size(); ++x) {
            if(mapa[y][x] == 'A') {
                // Centraliza na célula (x * CELL_SIZE + CELL_SIZE/2)
                start_x = x * CELL_SIZE + CELL_SIZE/2.0f;
                start_y = y * CELL_SIZE + CELL_SIZE/2.0f;
            }
        }
    }

    // Estado "Real" do veículo
    float true_pos_x = start_x;
    float true_pos_y = start_y;
    float true_ang = 0.0f;     // Começa apontando para Leste
    float velocidade = 20.0f;  // m/s
    float dt = 0.1f;           

    // Variáveis de estado para o filtro (EMA)
    float ema_pos_x = 0.0f;
    float ema_pos_y = 0.0f;
    float ema_ang_x = 0.0f;
    bool primeira_leitura = true;

    while (true) {
        // --- 1. Simulação da Física e Colisão ---
        
        // Calcula a próxima posição proposta
        float rad_ang = true_ang * M_PI / 180.0f;
        float next_x = true_pos_x + velocidade * std::cos(rad_ang) * dt;
        float next_y = true_pos_y + velocidade * std::sin(rad_ang) * dt;

        // Verifica se a próxima posição causaria colisão
        if (verificar_colisao(next_x, next_y, true_ang, mapa)) {
            // COLISÃO DETECTADA!
            // Lógica simples: Inverte a direção (gira 180 graus) e adiciona um pequeno desvio aleatório
            // para não ficar preso em loops perfeitos
            std::cout << "!!! COLISÃO DETECTADA em (" << next_x << ", " << next_y << ") !!! Virando..." << std::endl;
            true_ang += 180.0f + (std::rand() % 60 - 30); // 180 +/- 30 graus
            if (true_ang >= 360.0f) true_ang -= 360.0f;
            if (true_ang < 0.0f) true_ang += 360.0f;
            
            // Não atualiza x e y neste passo (o caminhão para momentaneamente para virar)
        } else {
            // Caminho livre, avança
            true_pos_x = next_x;
            true_pos_y = next_y;
            
            // Pequena variação aleatória na direção para simular imperfeição na direção
            // e ajudar a explorar o labirinto
            if (std::rand() % 10 == 0) { // 10% de chance de mudar levemente a direção
                 true_ang += (std::rand() % 10 - 5); // +/- 5 graus
            }
        }

        // --- 2. Leitura dos Sensores (Adiciona Ruído) ---
        float raw_pos_x_f = true_pos_x + noise_pos(generator);
        float raw_pos_y_f = true_pos_y + noise_pos(generator);
        float raw_ang_x_f = true_ang + noise_ang(generator);
        int raw_temp = static_cast<int>(temp_dist(generator));

        int raw_pos_x = static_cast<int>(raw_pos_x_f);
        int raw_pos_y = static_cast<int>(raw_pos_y_f);
        int raw_ang_x = static_cast<int>(raw_ang_x_f);

        // --- 3. Processamento (Filtro EMA) ---
        if (primeira_leitura) {
            // Na primeira leitura, inicializamos a média com o valor atual para evitar convergência lenta
            ema_pos_x = static_cast<float>(raw_pos_x);
            ema_pos_y = static_cast<float>(raw_pos_y);
            ema_ang_x = static_cast<float>(raw_ang_x);
            primeira_leitura = false;
        } else {
            // Nas leituras subsequentes, aplicamos o filtro EMA
            ema_pos_x = calcular_media_movel_exponencial(static_cast<float>(raw_pos_x), ema_pos_x);
            ema_pos_y = calcular_media_movel_exponencial(static_cast<float>(raw_pos_y), ema_pos_y);
            ema_ang_x = calcular_media_movel_exponencial(static_cast<float>(raw_ang_x), ema_ang_x);
        }

        // --- 4. Empacotamento e Envio ---
        DadosSensores novosDados;
        novosDados.i_temperatura = raw_temp;
        novosDados.i_posicao_x = static_cast<int>(ema_pos_x);
        novosDados.i_posicao_y = static_cast<int>(ema_pos_y);
        novosDados.i_angulo_x = static_cast<int>(ema_ang_x);
        novosDados.i_falha_eletrica = false; 
        novosDados.i_falha_hidraulica = false; 

        gerenciadorDados.setDados(novosDados);
        
        // Log simplificado
        // std::cout << "Pos: (" << novosDados.i_posicao_x << ", " << novosDados.i_posicao_y << ") Ang: " << novosDados.i_angulo_x << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

