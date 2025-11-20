#include "task_tratamento_sensores.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

// Constantes do filtro
const int N = 10; // Número de períodos para suavização
const float K = 2.0f / (1.0f + N); // Fator de suavização

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

void task_tratamento_sensores(GerenciadorDados& gerenciadorDados) {
    std::default_random_engine generator;
    std::uniform_real_distribution<float> temp_dist(-100, 200);
    std::uniform_real_distribution<float> pos_dist(0, 1000);
    std::uniform_real_distribution<float> ang_dist(0, 360);

    // Variáveis de estado para armazenar a média anterior de cada sensor
    float ema_pos_x = 0.0f;
    float ema_pos_y = 0.0f;
    float ema_ang_x = 0.0f;
    bool primeira_leitura = true;

    while (true) {
        // 1. Leitura bruta (Simulação)
        int raw_pos_x = pos_dist(generator);
        int raw_pos_y = pos_dist(generator);
        int raw_ang_x = ang_dist(generator);
        int raw_temp = temp_dist(generator);

        // 2. Processamento (Filtro)
        if (primeira_leitura) {
            // Na primeira leitura, a média é o próprio valor lido para evitar delay inicial
            ema_pos_x = static_cast<float>(raw_pos_x);
            ema_pos_y = static_cast<float>(raw_pos_y);
            ema_ang_x = static_cast<float>(raw_ang_x);
            primeira_leitura = false;
        } else {
            // Nas leituras subsequentes, usa a função auxiliar
            ema_pos_x = calcular_media_movel_exponencial(static_cast<float>(raw_pos_x), ema_pos_x);
            ema_pos_y = calcular_media_movel_exponencial(static_cast<float>(raw_pos_y), ema_pos_y);
            ema_ang_x = calcular_media_movel_exponencial(static_cast<float>(raw_ang_x), ema_ang_x);
        }

        // 3. Empacotamento dos dados
        DadosSensores novosDados;
        novosDados.i_temperatura = raw_temp;
        novosDados.i_posicao_x = static_cast<int>(ema_pos_x);
        novosDados.i_posicao_y = static_cast<int>(ema_pos_y);
        novosDados.i_angulo_x = static_cast<int>(ema_ang_x);
        novosDados.i_falha_eletrica = false; 
        novosDados.i_falha_hidraulica = false; 

        // 4. Envio para o Buffer (Produtor)
        gerenciadorDados.setDados(novosDados);
        
        // Log para visualização do teste (Simulação vs Média Móvel)
        std::cout << "[Sensor] Raw X: " << raw_pos_x << " -> Filtered X: " << novosDados.i_posicao_x << std::endl;
        // std::cout << "Raw Y: " << raw_pos_y << " -> Filtered Y: " << novosDados.i_posicao_y << std::endl;
        // std::cout << "Raw Ang: " << raw_ang_x << " -> Filtered Ang: " << novosDados.i_angulo_x << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Aumentei o tempo para facilitar a leitura no terminal
    }
}
