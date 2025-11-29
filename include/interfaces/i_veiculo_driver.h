#ifndef I_VEICULO_DRIVER_H
#define I_VEICULO_DRIVER_H

/**
 * @brief Interface para envio de comandos aos atuadores do veículo.
 * Segue o Princípio da Inversão de Dependência.
 */
class IVeiculoDriver {
public:
    virtual ~IVeiculoDriver() = default;

    /**
     * @brief Envia comandos de aceleração e direção.
     * @param aceleracao Percentual (-100 a 100).
     * @param direcao Graus (-180 a 180).
     */
    virtual void setAtuadores(int aceleracao, int direcao) = 0;
};

#endif // I_VEICULO_DRIVER_H