/**
 * @file gerenciador_dados.h
 * @brief Gerenciamento centralizado e thread-safe dos dados do veículo.
 */

#ifndef GERENCIADOR_DADOS_H
#define GERENCIADOR_DADOS_H

#include <boost/circular_buffer.hpp>
#include <mutex>
#include <condition_variable>
#include "dados.h"

/**
 * @class GerenciadorDados
 * @brief Classe que implementa o padrão Monitor para gerenciar o acesso aos dados compartilhados.
 * 
 * Esta classe encapsula o buffer circular de dados dos sensores, bem como o estado
 * atual do veículo e os comandos do operador. Fornece métodos thread-safe para
 * leitura e escrita, garantindo a integridade dos dados em um ambiente multithread.
 */
class GerenciadorDados {
private:
    const int TAMANHO_BUFFER = 200; ///< Tamanho máximo do buffer circular.
    boost::circular_buffer<DadosSensores> dadosSensores; ///< Buffer circular para armazenar histórico de leituras.
    EstadoVeiculo estadoVeiculo; ///< Estado atual do veículo.
    ComandosOperador comandosOperador; ///< Comandos atuais do operador.

    mutable std::mutex mtx; ///< Mutex para exclusão mútua.
    std::condition_variable cv_dados; ///< Variável de condição para sincronização de produtores/consumidores.

public:
    /**
     * @brief Construtor padrão. Inicializa o buffer e os estados.
     */
    GerenciadorDados();

    /**
     * @brief Insere novos dados de sensores no buffer circular.
     * 
     * Atua como Produtor. Se o buffer estiver cheio, sobrescreve o dado mais antigo.
     * Notifica consumidores aguardando dados.
     * 
     * @param dados Estrutura contendo as leituras dos sensores.
     */
    void setDados(const DadosSensores& dados);

    /**
     * @brief Recupera o dado mais antigo do buffer circular.
     * 
     * Atua como Consumidor. Bloqueia a thread se o buffer estiver vazio até que
     * novos dados estejam disponíveis.
     * 
     * @return DadosSensores Estrutura com os dados lidos.
     */
    DadosSensores getDados();

    /**
     * @brief Atualiza o estado operacional do veículo.
     * @param estado Novo estado do veículo.
     */
    void setEstadoVeiculo(const EstadoVeiculo& estado);

    /**
     * @brief Obtém o estado operacional atual do veículo.
     * @return EstadoVeiculo Estado atual.
     */
    EstadoVeiculo getEstadoVeiculo() const;

    /**
     * @brief Define os comandos do operador.
     * @param comandos Novos comandos do operador.
     */
    void setComandosOperador(const ComandosOperador& comandos);

    /**
     * @brief Obtém os comandos atuais do operador.
     * @return ComandosOperador Comandos atuais.
     */
    ComandosOperador getComandosOperador() const;

    /**
     * @brief Atualiza atomicamente o estado do veículo (alias para setEstadoVeiculo).
     * @param estado Novo estado.
     */
    void atualizarEstadoVeiculo(const EstadoVeiculo& estado);

    /**
     * @brief Atualiza atomicamente os comandos do operador (alias para setComandosOperador).
     * @param comandos Novos comandos.
     */
    void atualizarComandosOperador(const ComandosOperador& comandos);

    /**
     * @brief Obtém o número atual de elementos no buffer.
     * @return int Número de elementos no buffer.
     */
    int getContadorDados() const;
};

#endif // GERENCIADOR_DADOS_H
