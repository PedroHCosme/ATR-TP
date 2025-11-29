/**
 * @file task_logica_comando.h
 * @brief Definição da tarefa de lógica de comando e supervisão.
 */

#ifndef TASK_LOGICA_COMANDO_H
#define TASK_LOGICA_COMANDO_H

#include "gerenciador_dados.h"
#include "eventos_sistema.h"

class IVeiculoDriver;

/**
 * @brief Tarefa responsável pela lógica de comando central e tomada de decisão.
 *
 * Atua como o "cérebro" do sistema, integrando entradas do operador, dados de sensores
 * e eventos do sistema para determinar o modo de operação (Manual/Automático) e
 * gerenciar transições de estado.
 *
 * @param gerenciadorDados Referência para o gerenciador de dados compartilhado.
 * @param eventos Referência para o sistema de eventos e falhas.
 * @param driver Referência para o driver universal (interface de controle do veículo).
 */
void task_logica_comando(GerenciadorDados& gerenciadorDados, EventosSistema& eventos, IVeiculoDriver& driver);

#endif // TASK_LOGICA_COMANDO_H
