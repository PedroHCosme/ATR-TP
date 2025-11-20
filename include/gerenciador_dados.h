#ifndef GERENCIADOR_DADOS_H
#define GERENCIADOR_DADOS_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include "dados.h"

// Declaração da classe GerenciadorDados
class GerenciadorDados {
private:
    const int TAMANHO_BUFFER = 200;
    std::vector<DadosSensores> dadosSensores;
    int indice_escrita = 0;
    int indice_leitura = 0;
    int contador_dados = 0;

    EstadoVeiculo estadoVeiculo;
    ComandosOperador comandosOperador;

    mutable std::mutex mtx;
    std::condition_variable cv_dados;

public:
    GerenciadorDados();

    void setDados(const DadosSensores& dados);
    DadosSensores getDados();

    void setEstadoVeiculo(const EstadoVeiculo& estado);
    EstadoVeiculo getEstadoVeiculo() const;

    void setComandosOperador(const ComandosOperador& comandos);
    ComandosOperador getComandosOperador() const;

    void atualizarEstadoVeiculo(const EstadoVeiculo& estado);
    void atualizarComandosOperador(const ComandosOperador& comandos);
};

#endif // GERENCIADOR_DADOS_H
