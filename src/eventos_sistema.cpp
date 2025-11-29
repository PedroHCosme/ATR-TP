#include "eventos_sistema.h"
#include <atomic>

void EventosSistema::sinalizar_falha(int codigo) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (!falha_critica_detectada) {
        falha_critica_detectada = true;
        codigo_falha = codigo;
        
        cv_falha.notify_all(); 
    }
}

bool EventosSistema::verificar_estado_falha() {
    std::lock_guard<std::mutex> lock(mtx);
    return falha_critica_detectada;
}

void EventosSistema::resetar_falhas() {
    std::lock_guard<std::mutex> lock(mtx);
    falha_critica_detectada = false;
    codigo_falha = 0;
}