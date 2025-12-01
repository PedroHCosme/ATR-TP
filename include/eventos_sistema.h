/**
 * @file eventos_sistema.h
 * @brief Gerenciamento de eventos e sinalização de falhas do sistema.
 * @author Pedro Cosme
 * @date 2025
 */

#ifndef EVENTOS_SISTEMA_H
#define EVENTOS_SISTEMA_H

#include <condition_variable>
#include <mutex>

/**
 * @class EventosSistema
 * @brief Classe responsável pela sincronização e notificação de eventos
 * críticos do sistema.
 *
 * Esta classe atua como um monitor thread-safe para gerenciar o estado de
 * falhas do sistema. Permite que tarefas sinalizem a ocorrência de falhas e que
 * outras tarefas verifiquem ou aguardem por esses eventos.
 */
class EventosSistema {
private:
  std::mutex mtx; ///< Mutex para proteção de acesso concorrente.
  std::condition_variable
      cv_falha; ///< Variável de condição para notificação de falhas.
  bool falha_critica_detectada =
      false;            ///< Flag indicando se uma falha crítica está ativa.
  int codigo_falha = 0; ///< Código identificador da falha (Ex: 1=Temp,
                        ///< 2=Eletrica, 3=Hidraulica).

public:
  /**
   * @brief Sinaliza a ocorrência de uma falha no sistema.
   *
   * Este método deve ser chamado pelas tarefas de monitoramento quando uma
   * anomalia é detectada. Ele atualiza o estado interno e notifica as threads
   * interessadas.
   *
   * @param codigo Código inteiro representando o tipo de falha.
   */
  void sinalizar_falha(int codigo);

  /**
   * @brief Verifica se existe uma falha crítica ativa no momento.
   *
   * @return true Se houver uma falha crítica não tratada.
   * @return false Se o sistema estiver operando normalmente.
   */
  bool verificar_estado_falha();

  /**
   * @brief Reseta o estado de falha do sistema.
   *
   * Limpa a flag de falha crítica e o código de erro, permitindo que o sistema
   * retorne à operação normal. Geralmente chamado após o tratamento da falha.
   */
  /**
   * @brief Retorna o código da falha atual.
   * @return int Código da falha.
   */
  int getCodigoFalha();

  void resetar_falhas();
};

#endif // EVENTOS_SISTEMA_H