#include "task_collision_avoidance.h"
#include "utils/sleep_asynch.h"
#include <chrono>
#include <iostream>
#include <thread>

// Configurações de Segurança
const float SAFE_DISTANCE_METERS = 10.0f; // Distância mínima segura
const int FAULT_CODE_OBSTACLE = 4;        // Código de falha para obstáculo

void task_collision_avoidance(GerenciadorDados &dados, EventosSistema &eventos,
                              IVeiculoDriver &driver) {
  std::cout << "[CAS] Task de Prevenção de Colisão INICIADA." << std::endl;

  while (true) {
    // 1. Leitura de Alta Prioridade (Snapshot)
    DadosSensores estado = dados.lerUltimoEstado();

    // 2. Lógica de Segurança (Safety Kernel)
    // Se a distância for menor que o limiar seguro, ativa freio de emergência
    float distancia = estado.i_lidar_distancia;
    if (distancia < SAFE_DISTANCE_METERS) {
      // --- SITUAÇÃO DE PERIGO DETECTADA ---
      // Violação de segurança detectada!
      std::cout << "[CAS] PERIGO! Obstaculo detectado a " << distancia
                << "m (Limiar: " << SAFE_DISTANCE_METERS << "m)" << std::endl;

      // A. Override Imediato dos Atuadores (Freio de Emergência)
      // Envia comando direto para o driver, ignorando a task de navegação
      // FIX: Frenagem ativa (aceleração negativa) para parar caminhão pesado
      // Ação de mitigação: Freio total
      driver.setAtuadores(-100,
                          0); // -100% aceleração (freio), mantém direção para o
                              // driver, ignorando a task de navegação

      // B. Sinalização de Falha (Intertrava o sistema)
      // Isso fará com que a task de navegação pare de enviar comandos também
      if (!eventos.verificar_estado_falha()) {
        std::cerr << "!!! [CAS] OBSTÁCULO DETECTADO ("
                  << estado.i_lidar_distancia << "m) !!! PARADA DE EMERGÊNCIA."
                  << std::endl;
        eventos.sinalizar_falha(FAULT_CODE_OBSTACLE);
      }

    } else {
      // --- SITUAÇÃO SEGURA ---
      // Nenhuma ação necessária, deixa o controle com a navegação.
      // Opcional: Se a falha estava ativa e o obstáculo sumiu, poderíamos
      // tentar resetar? Por segurança, exigimos reset manual do operador (botão
      // R).
    }

    // 3. Loop de Alta Frequência (20Hz - 50ms)
    // Deve ser rápido o suficiente para reagir antes da colisão
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}
