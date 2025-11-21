/**
 * @file simulacao_mina.h
 * @brief Motor de física e simulação do ambiente da mina.
 */

#ifndef SIMULACAO_MINA_H
#define SIMULACAO_MINA_H

#include <vector>
#include <mutex>
#include <cmath>
#include <random> 
#include "dados.h"

/**
 * @struct CaminhaoFisico
 * @brief Representa o estado físico completo de um caminhão na simulação.
 * 
 * Contém variáveis de estado contínuas (posição float, velocidade, temperatura)
 * que representam a "verdade" do mundo físico, antes de serem discretizadas
 * ou ruidosas pelos sensores.
 */
struct CaminhaoFisico {
    int id;                         ///< Identificador único do caminhão.
    float i_posicao_x, i_posicao_y; ///< Posição global em metros.
    float i_angulo_x;               ///< Orientação em graus (0 = Leste).
    float velocidade;               ///< Velocidade linear atual em m/s.
    float o_aceleracao;             ///< Comando de aceleração atual (m/s²).
    float o_direcao;                ///< Comando de direção atual (graus).
    float i_temperatura;            ///< Temperatura interna do motor (°C).
    bool i_falha_eletrica;          ///< Flag de falha elétrica injetada.
    bool i_falha_hidraulica;        ///< Flag de falha hidráulica injetada.
    int temperatuta_ambiente;       ///< Temperatura ambiente local (°C).
};

/**
 * @class SimulacaoMina
 * @brief Motor de simulação física do ambiente de mineração.
 * 
 * Responsável por manter o estado "real" do mundo, calcular a física de movimento
 * (modelo de bicicleta), termodinâmica básica e detecção de colisões com o mapa.
 */
class SimulacaoMina {
private:
    std::vector<CaminhaoFisico> frota; ///< Lista de caminhões na simulação.
    const std::vector<std::vector<char>>& mapa; ///< Referência ao grid do mapa (read-only).
    mutable std::mutex mtx_simulacao; ///< Mutex para proteger o estado da simulação.
    float dt; ///< Passo de tempo da simulação (delta time).

public:
    /**
     * @brief Construtor da simulação.
     * 
     * @param mapa_ref Referência para o mapa gerado (grid de caracteres).
     * @param num_caminhoes Número de caminhões a serem instanciados.
     */
    SimulacaoMina(const std::vector<std::vector<char>>& mapa_ref, int num_caminhoes);

    /**
     * @brief Avança a simulação em um passo de tempo (dt).
     * 
     * Atualiza a física de todos os veículos, incluindo movimento e temperatura.
     * Deve ser chamado periodicamente por uma thread dedicada.
     */
    void atualizar_passo_tempo();

    /**
     * @brief Define os comandos de atuação para um caminhão específico.
     * 
     * @param id_caminhao ID do caminhão alvo.
     * @param aceleracao Valor de aceleração comandado.
     * @param direcao Valor de direção comandado.
     */
    void setComandoAtuador(int id_caminhao, int aceleracao, int direcao);

    /**
     * @brief Obtém o estado físico real de um caminhão.
     * 
     * Usado pelos sensores para gerar leituras (que podem ter ruído adicionado posteriormente).
     * 
     * @param id_caminhao ID do caminhão.
     * @return CaminhaoFisico Cópia do estado atual do caminhão.
     */
    CaminhaoFisico getEstadoReal(int id_caminhao);

private:
    /**
     * @brief Aplica o modelo cinemático de bicicleta para atualizar posição e orientação.
     * @param caminhao Referência ao objeto do caminhão a ser atualizado.
     */
    void modelo_bicicleta(CaminhaoFisico& caminhao);

    /**
     * @brief Aplica um modelo termodinâmico simples para atualizar a temperatura do motor.
     * @param caminhao Referência ao objeto do caminhão a ser atualizado.
     */
    void modelo_maquina_termica(CaminhaoFisico& caminhao);

    /**
     * @brief Verifica colisão do veículo com obstáculos do mapa.
     * 
     * Utiliza uma Bounding Box orientada para verificar intersecção com células de parede.
     * 
     * @param x Posição X proposta.
     * @param y Posição Y proposta.
     * @param angulo Ângulo proposto.
     * @return true Se houver colisão.
     * @return false Se o caminho estiver livre.
     */
    bool verificar_colisao(float x, float y, float angulo);
};

#endif // SIMULACAO_MINA_H