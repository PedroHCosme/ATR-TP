#ifndef TASK_PLANEJAMENTO_ROTA_H
#define TASK_PLANEJAMENTO_ROTA_H

#include "drivers/mqtt_driver.h"
#include "gerenciador_dados.h"

/**
 * @brief Tarefa de Planejamento de Rota ("O General").
 *
 * Gerencia uma fila de waypoints e atualiza o ObjetivoNavegacao
 * no GerenciadorDados para que a tarefa de navegação ("O Piloto") possa seguir.
 *
 * @param dados Gerenciador de dados compartilhado.
 * @param mqtt Driver MQTT para receber missões futuras (Fase 2).
 */
void task_planejamento_rota(GerenciadorDados &dados, MqttDriver &mqtt);

#endif // TASK_PLANEJAMENTO_ROTA_H
