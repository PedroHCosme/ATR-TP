#ifndef GERENCIADOR_DADOS_H
#define GERENCIADOR_DADOS_H

#include <vector>
#include <mutex>
#include <condition_variable>

// Estruturas de dados
struct DadosSensores
{
    int i_posicao_x;
    int i_posicao_y;
    int i_angulo_x;
    int i_temperatura;
    bool i_falha_eletrica;
    bool i_falha_hidraulica;
    int o_aceleracao;
    int o_direcao;
};

struct EstadoVeiculo
{
    bool e_defeito;
    bool e_automatico;
};

struct ComandosOperador
{
    bool c_automatico;
    bool c_man;
    bool c_rearme;
    bool c_acelerar;
    bool c_direita;
    bool c_esquerda;
};

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
