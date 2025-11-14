#include "gerenciador_dados.h"
#include <iostream>

/**
 * @brief Construtor da classe GerenciadorDados.
 * Inicializa o buffer de dados de sensores com um tamanho pré-definido.
 */
GerenciadorDados::GerenciadorDados() {
    dadosSensores.resize(TAMANHO_BUFFER);
    // Inicialização de outros membros se necessário
}

/**
 * @brief Adiciona dados de sensores ao buffer circular.
 * Esta função é segura para threads. Se o buffer estiver cheio, a thread que chama aguardará até que haja espaço.
 * Notifica um consumidor em espera após adicionar os dados.
 * @param dados Os dados dos sensores a serem adicionados.
 */
void GerenciadorDados::setDados(const DadosSensores& dados) {
    std::unique_lock<std::mutex> lock(mtx);
    cv_dados.wait(lock, [this] { return contador_dados < TAMANHO_BUFFER; });
    dadosSensores[indice_escrita] = dados;
    indice_escrita = (indice_escrita + 1) % TAMANHO_BUFFER;
    contador_dados++;
    cv_dados.notify_one();
}

/**
 * @brief Obtém dados de sensores do buffer circular.
 * Esta função é segura para threads. Se o buffer estiver vazio, a thread que chama aguardará até que haja dados disponíveis.
 * Notifica um produtor em espera após remover os dados.
 * @return Os dados dos sensores lidos do buffer.
 */
DadosSensores GerenciadorDados::getDados() {
    std::unique_lock<std::mutex> lock(mtx);
    cv_dados.wait(lock, [this] { return contador_dados > 0; });
    DadosSensores dados = dadosSensores[indice_leitura];
    indice_leitura = (indice_leitura + 1) % TAMANHO_BUFFER;
    contador_dados--;
    cv_dados.notify_one();
    return dados;
}

/**
 * @brief Define o estado do veículo.
 * @param estado O novo estado do veículo.
 */
void GerenciadorDados::setEstadoVeiculo(const EstadoVeiculo& estado) {
    std::lock_guard<std::mutex> lock(mtx);
    estadoVeiculo = estado;
}

/**
 * @brief Obtém o estado atual do veículo.
 * @return O estado atual do veículo.
 */
EstadoVeiculo GerenciadorDados::getEstadoVeiculo() const {
    std::lock_guard<std::mutex> lock(mtx);
    return estadoVeiculo;
}

/**
 * @brief Define os comandos do operador.
 * @param comandos Os novos comandos do operador.
 */
void GerenciadorDados::setComandosOperador(const ComandosOperador& comandos) {
    std::lock_guard<std::mutex> lock(mtx);
    comandosOperador = comandos;
}

/**
 * @brief Obtém os comandos atuais do operador.
 * @return Os comandos atuais do operador.
 */
ComandosOperador GerenciadorDados::getComandosOperador() const {
    std::lock_guard<std::mutex> lock(mtx);
    return comandosOperador;
}

/**
 * @brief Atualiza o estado do veículo.
 * @param estado O novo estado do veículo a ser atualizado.
 */
void GerenciadorDados::atualizarEstadoVeiculo(const EstadoVeiculo& estado) {
    std::lock_guard<std::mutex> lock(mtx);
    estadoVeiculo = estado;
}

/**
 * @brief Atualiza os comandos do operador.
 * @param comandos Os novos comandos do operador a serem atualizados.
 */
void GerenciadorDados::atualizarComandosOperador(const ComandosOperador& comandos) {
    std::lock_guard<std::mutex> lock(mtx);
    comandosOperador = comandos;
}
