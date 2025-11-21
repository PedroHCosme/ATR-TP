#ifndef EVENTOS_SISTEMA_H
#define EVENTOS_SISTEMA_H

#include <mutex>
#include <condition_variable>

class EventosSistema {
private:
    std::mutex mtx;
    std::condition_variable cv_falha;
    bool falha_critica_detectada = false;
    int codigo_falha = 0; // Ex: 1=Temp, 2=Eletrica, 3=Hidraulica

public:
    // Chamado pelo Monitoramento de Falhas
    void sinalizar_falha(int codigo);

    bool verificar_estado_falha();

    void resetar_falhas();
};