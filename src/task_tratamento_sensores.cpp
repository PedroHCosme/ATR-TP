#include "task_tratamento_sensores.h"
#include "simulacao_mina.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <vector>

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

/**
 * @brief Tarefa responsável pela leitura e tratamento dos dados dos sensores.
 * 
 * Esta função simula a leitura de sensores de um caminhão em movimento dentro do mapa gerado.
 * Agora ela atua como uma "leitora burra", consultando a simulação física.
 * 
 * @param gerenciadorDados Referência para o gerenciador de dados compartilhado.
 * @param simulacao Referência para a simulação física.
 * @param id_caminhao ID do caminhão a ser monitorado.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados, ISensorDriver& driver, int id_caminhao) {
    std::default_random_engine generator;
    
    // Configuração dos geradores de ruído
    std::normal_distribution<float> noise_pos(0.0, 1.0); 
    std::normal_distribution<float> noise_ang(0.0, 2.0); 
    std::normal_distribution<float> noise_temp(0.0, 0.5);

    // Variáveis de estado para o filtro (EMA)
    float ema_pos_x = 0.0f;
    float ema_pos_y = 0.0f;
    float ema_ang_x = 0.0f;
    bool primeira_leitura = true;

    while (true) {
        // --- 1. Leitura do Estado Real da Simulação ---
        CaminhaoFisico estadoReal = driver.readSensorData(id_caminhao);
        
        // --- 2. Aplicação de Ruído (Simulação de Sensores) ---
        float raw_pos_x_f = estadoReal.i_posicao_x + noise_pos(generator);
        float raw_pos_y_f = estadoReal.i_posicao_y + noise_pos(generator);
        float raw_ang_x_f = estadoReal.i_angulo_x + noise_ang(generator);
        float raw_temp_f = estadoReal.i_temperatura + noise_temp(generator);

        int raw_pos_x = static_cast<int>(raw_pos_x_f);
        int raw_pos_y = static_cast<int>(raw_pos_y_f);
        int raw_ang_x = static_cast<int>(raw_ang_x_f);
        int raw_temp = static_cast<int>(raw_temp_f);

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
        
        // Debug: Print da temperatura lida pelo sensor
        std::cout << "[DEBUG-SENSOR] Temp Real: " << estadoReal.i_temperatura 
                  << " | Com Ruido: " << raw_temp_f << " | Inteiro: " << raw_temp << std::endl;
        
        novosDados.i_posicao_x = static_cast<int>(ema_pos_x);
        novosDados.i_posicao_y = static_cast<int>(ema_pos_y);
        novosDados.i_angulo_x = static_cast<int>(ema_ang_x);
        novosDados.i_velocidade = static_cast<int>(estadoReal.velocidade); // Copia do físico para o sensor
        novosDados.i_falha_eletrica = estadoReal.i_falha_eletrica; 
        novosDados.i_falha_hidraulica = estadoReal.i_falha_hidraulica; 

        gerenciadorDados.setDados(novosDados);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

