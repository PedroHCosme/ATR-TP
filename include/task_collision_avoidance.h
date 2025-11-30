#ifndef TASK_COLLISION_AVOIDANCE_H
#define TASK_COLLISION_AVOIDANCE_H

#include "eventos_sistema.h"
#include "gerenciador_dados.h"
#include "interfaces/i_veiculo_driver.h"

/**
 * @brief Tarefa de Segurança Crítica: Sistema de Prevenção de Colisão (CAS).
 *
 * Monitora o sensor LiDAR virtual e, se a distância for menor que o limite de
 * segurança, toma o controle dos atuadores (Override) para parar o veículo e
 * sinaliza falha.
 *
 * @param dados Gerenciador de dados para leitura de estado.
 * @param eventos Sistema de eventos para sinalizar falha.
 * @param driver Interface do driver de veículo para envio direto de comando de
 * parada (Bypass).
 */
void task_collision_avoidance(GerenciadorDados &dados, EventosSistema &eventos,
                              IVeiculoDriver &driver);

#endif // TASK_COLLISION_AVOIDANCE_H
