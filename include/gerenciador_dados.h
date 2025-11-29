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

class GerenciadorDados {
private:
    const int TAMANHO_BUFFER = 200;
    
    // STREAM: Histórico para log/auditoria (FIFO)
    boost::circular_buffer<DadosSensores> bufferHistorico;
    
    // SNAPSHOT: Estado mais recente para controle em tempo real (LVC - Last Value Cache)
    DadosSensores ultimoEstado;

    EstadoVeiculo estadoVeiculo;
    ComandosOperador comandosOperador;
    ComandosAtuador comandosAtuador;

    mutable std::mutex mtx; 
    std::condition_variable cv_dados; 

public:
    GerenciadorDados();

    /**
     * @brief PRODUTOR: Insere dados no sistema.
     * Atualiza tanto o histórico (buffer) quanto o snapshot (estado atual).
     * Thread-safe e não bloqueante (sobrescreve se buffer cheio).
     */
    void setDados(const DadosSensores& dados);

    /**
     * @brief CONSUMIDOR DE LOG: Retira o dado mais antigo do histórico.
     * Use apenas para tarefas que precisam salvar o histórico (Coletor de Dados).
     * BLOQUEANTE: Espera se o buffer estiver vazio.
     */
    DadosSensores consumirDados();

    /**
     * @brief LEITOR DE CONTROLE: Lê o estado mais recente sem remover do buffer.
     * Use para Navegação e Lógica de Comando.
     * NÃO BLOQUEANTE: Retorna imediatamente a última cópia conhecida.
     */
    DadosSensores lerUltimoEstado() const;

    // --- Getters e Setters de Estado (Mantidos iguais) ---
    void setEstadoVeiculo(const EstadoVeiculo& estado);
    EstadoVeiculo getEstadoVeiculo() const;

    void setComandosOperador(const ComandosOperador& comandos);
    ComandosOperador getComandosOperador() const;

    void atualizarEstadoVeiculo(const EstadoVeiculo& estado);
    void atualizarComandosOperador(const ComandosOperador& comandos);

    // --- Acesso Direto à Memória de Atuação (Novo) ---
    void setComandosAtuador(const ComandosAtuador& comandos);
    ComandosAtuador getComandosAtuador() const;

    // Retorna tamanho do buffer (para monitoramento)
    int getContadorDados() const;
};

#endif // GERENCIADOR_DADOS_H