/**
 * @file task_monitoramento_falhas.h
 * @brief Monitoramento de saúde e segurança do veículo.
 */

#ifndef TASK_MONITORAMENTO_FALHAS_H
#define TASK_MONITORAMENTO_FALHAS_H

#include "gerenciador_dados.h"
#include "eventos_sistema.h"

class ISensorDriver;

/**
 * @brief Tarefa de Segurança (Watchdog).
 * 
 * Monitora continuamente o Snapshot (estado mais recente) dos sensores.
 * Se detectar temperatura crítica ou flags de falha elétrica/hidráulica,
 * sinaliza o sistema de eventos (EventosSistema).
 * 
 * @param gerenciadorDados Referência para o banco de dados central.
 * @param eventos Referência para o sistema de eventos.
 * @param driver Referência para o driver de sensores (leitura direta).
 */
void task_monitoramento_falhas(GerenciadorDados& gerenciadorDados, EventosSistema& eventos, ISensorDriver& driver);

#endif // TASK_MONITORAMENTO_FALHAS_H