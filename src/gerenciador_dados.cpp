#include "gerenciador_dados.h"
#include <iostream>

GerenciadorDados::GerenciadorDados() : bufferHistorico(TAMANHO_BUFFER) {
    // Inicializa o snapshot zerado para evitar leitura de lixo antes do primeiro sensor
    ultimoEstado = {0}; 
}

void GerenciadorDados::setDados(const DadosSensores& dados) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // 1. Alimenta o Stream (Histórico)
    // O circular_buffer cuida de sobrescrever o antigo se estiver cheio
    bufferHistorico.push_back(dados);
    
    // 2. Atualiza o Snapshot (Tempo Real)
    ultimoEstado = dados;
    
    // Notifica quem estiver esperando por dados novos no histórico (Logger)
    cv_dados.notify_one();
}

DadosSensores GerenciadorDados::consumirDados() {
    std::unique_lock<std::mutex> lock(mtx);
    
    // Bloqueia se não houver histórico para consumir
    cv_dados.wait(lock, [this] { return !bufferHistorico.empty(); });
    
    DadosSensores dados = bufferHistorico.front();
    bufferHistorico.pop_front(); // Remove da fila!
    
    return dados;
}

DadosSensores GerenciadorDados::lerUltimoEstado() const {
    std::lock_guard<std::mutex> lock(mtx);
    // Apenas lê a variável. Rápido e não interfere no buffer.
    return ultimoEstado;
}

// --- Métodos Boilerplate (Mantidos) ---

void GerenciadorDados::setEstadoVeiculo(const EstadoVeiculo& estado) {
    std::lock_guard<std::mutex> lock(mtx);
    estadoVeiculo = estado;
}

EstadoVeiculo GerenciadorDados::getEstadoVeiculo() const {
    std::lock_guard<std::mutex> lock(mtx);
    return estadoVeiculo;
}

void GerenciadorDados::setComandosOperador(const ComandosOperador& comandos) {
    std::lock_guard<std::mutex> lock(mtx);
    comandosOperador = comandos;
}

ComandosOperador GerenciadorDados::getComandosOperador() const {
    std::lock_guard<std::mutex> lock(mtx);
    return comandosOperador;
}

void GerenciadorDados::atualizarEstadoVeiculo(const EstadoVeiculo& estado) {
    std::lock_guard<std::mutex> lock(mtx);
    estadoVeiculo = estado;
}

void GerenciadorDados::atualizarComandosOperador(const ComandosOperador& comandos) {
    std::lock_guard<std::mutex> lock(mtx);
    comandosOperador = comandos;
}

int GerenciadorDados::getContadorDados() const {
    std::lock_guard<std::mutex> lock(mtx);
    return bufferHistorico.size();
}