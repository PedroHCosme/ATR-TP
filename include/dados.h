#ifndef DADOS_H
#define DADOS_H

/**
 * @struct DadosSensores
 * @brief Armazena os dados lidos dos sensores do veículo.
 */
struct DadosSensores
{
    int id; // Identificador do veículo
    
    int i_posicao_x; // Posição do veículo no eixo x, com relação 
                    // a um referencial absoluto em solo obtido
                    // pelos sensores de posição.
    int i_posicao_y; // Posição do veículo no eixo y, com relação 
                    // a um referencial absoluto em solo obtido
                    // pelos sensores de posição.

    int i_angulo_x; // Direção angular da frente do veículo com
                    // relação à direção leste (leste = ângulo
                    // zero), obtido pelos sensores inerciais.

    int i_temperatura; // Temperatura do motor (varia entre -100 e
                    // +200). Essa temperatura possui um nível
                    // de alerta se 𝑇 > 95 °C e gera defeito se
                    // 𝑇 > 120 °C

    bool i_falha_eletrica; // Se for true, indica presença de falha no
                            // sistema elétrico do veículo
    bool i_falha_hidraulica; // Se for true, indica presença de falha no
                            // sistema hidráulico do veículo

    int o_aceleracao; //Determina a aceleração do veículo em
                      //   percentual (-100 a 100%)

    int o_direcao; // Determina a direção do veículo em graus (-180 a 180°). Ao acelerar,
                   // o veículo se moverá nessa direção.
};

/**
 * @struct EstadoVeiculo
 * @brief Armazena o estado operacional do veículo.
 */
struct EstadoVeiculo
{
    bool e_defeito; //Estado que identifica a presença de
                    // defeito ou defeito não reconhecido pelo
                    // operador (1:defeito, 0: sem defeito)

    bool e_automatico; //Estado que identifica o modo de operação
                        // do veículo (0: manual, 1: automático)
};

/**
 * @struct ComandosOperador
 * @brief Armazena os comandos recebidos do operador do veículo.
 */
struct ComandosOperador
{
    bool c_automatico; // Se true, indica que o operador deseja
                        // colocar o veículo em modo automático.
                        // Se false, indica que o operador deseja
                        // colocar o veículo em modo manual.
    
    bool c_man; // Se true, indica que o operador deseja
                        // colocar o veículo em modo manual.

    bool c_rearme; //Comando para rearmar algum defeito que
                   // tenha ocorrido no caminhão.

    bool c_acelerar; // Comando para acelerar o veículo.

    bool c_direita; // Comando para virar o veículo à direita.
    bool c_esquerda; // Comando para virar o veículo à esquerda.
};

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
    int temperatura_ambiente;       ///< Temperatura ambiente local (°C).
};


#endif // DADOS_H
