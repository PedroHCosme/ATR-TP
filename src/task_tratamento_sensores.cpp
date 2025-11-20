#include "task_tratamento_sensores.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>

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
 * Esta função simula a leitura de sensores de um caminhão em movimento,
 * aplica um ruído gaussiano para simular imprecisões reais e, em seguida,
 * utiliza um filtro de Média Móvel Exponencial para suavizar os dados
 * antes de enviá-los para o buffer circular.
 * 
 * A simulação cinemática considera um veículo movendo-se a velocidade constante
 * e realizando curvas suaves.
 * 
 * @param gerenciadorDados Referência para o gerenciador de dados compartilhado.
 */
void task_tratamento_sensores(GerenciadorDados& gerenciadorDados) {
    std::default_random_engine generator;
    
    // Configuração dos geradores de ruído (simulação de erro de sensor)
    std::normal_distribution<float> noise_pos(0.0, 5.0); // Ruído gaussiano na posição (desvio padrão de 5m)
    std::normal_distribution<float> noise_ang(0.0, 2.0); // Ruído gaussiano no ângulo (desvio padrão de 2 graus)
    std::uniform_real_distribution<float> temp_dist(80, 100); // Temperatura do motor variando entre 80 e 100 graus

    // Estado "Real" do veículo (Simulação da Física)
    float true_pos_x = 0.0f;
    float true_pos_y = 0.0f;
    float true_ang = 0.0f;     // Ângulo em graus
    float velocidade = 10.0f;  // m/s (aprox. 36 km/h)
    float dt = 0.1f;           // Intervalo de tempo da simulação (s)

    // Variáveis de estado para o filtro (EMA)
    float ema_pos_x = 0.0f;
    float ema_pos_y = 0.0f;
    float ema_ang_x = 0.0f;
    bool primeira_leitura = true;

    while (true) {
        // --- 1. Simulação da Física do Caminhão (Movimento Suave) ---
        // Atualiza o ângulo real: o caminhão vira 2 graus por segundo
        true_ang += 2.0f * dt; 
        if (true_ang >= 360.0f) true_ang -= 360.0f;

        // Converte para radianos para cálculos trigonométricos
        float rad_ang = true_ang * M_PI / 180.0f;
        
        // Atualiza posição real baseada na velocidade e direção atual
        true_pos_x += velocidade * std::cos(rad_ang) * dt;
        true_pos_y += velocidade * std::sin(rad_ang) * dt;

        // --- 2. Leitura dos Sensores (Adiciona Ruído ao valor Real) ---
        // Simula a leitura do sensor adicionando ruído ao valor físico real
        float raw_pos_x_f = true_pos_x + noise_pos(generator);
        float raw_pos_y_f = true_pos_y + noise_pos(generator);
        float raw_ang_x_f = true_ang + noise_ang(generator);
        int raw_temp = static_cast<int>(temp_dist(generator));

        // Converte para inteiro conforme esperado pela estrutura de dados
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

        // --- 4. Empacotamento dos dados ---
        DadosSensores novosDados;
        novosDados.i_temperatura = raw_temp;
        novosDados.i_posicao_x = static_cast<int>(ema_pos_x); // Envia o dado filtrado
        novosDados.i_posicao_y = static_cast<int>(ema_pos_y); // Envia o dado filtrado
        novosDados.i_angulo_x = static_cast<int>(ema_ang_x);  // Envia o dado filtrado
        novosDados.i_falha_eletrica = false; 
        novosDados.i_falha_hidraulica = false; 

        // --- 5. Envio para o Buffer (Produtor) ---
        gerenciadorDados.setDados(novosDados);
        
        // Log informativo
        std::cout << "[Sensor] Real X: " << true_pos_x 
                  << " | Raw X: " << raw_pos_x 
                  << " -> Filtered X: " << novosDados.i_posicao_x << std::endl;

        // Aguarda o intervalo de tempo definido (simula a taxa de amostragem)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

