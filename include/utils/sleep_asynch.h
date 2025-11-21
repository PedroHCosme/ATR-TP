/**
 * @file sleep_asynch.h
 * @brief Utilitário para operações de sleep assíncrono usando Boost.Asio.
 */

#ifndef SLEEP_ASYNCH_H
#define SLEEP_ASYNCH_H

#include <chrono>
#include <functional>
#include <boost/asio.hpp>

/**
 * @class SleepAsynch
 * @brief Classe auxiliar para gerenciar temporizadores assíncronos.
 * 
 * Permite agendar a execução de callbacks após um determinado período de tempo,
 * sem bloquear a thread de execução, utilizando o io_context do Boost.Asio.
 */
class SleepAsynch {
public:
    /**
     * @brief Construtor.
     * @param io Referência para o contexto de I/O do Boost.Asio.
     */
    explicit SleepAsynch(boost::asio::io_context& io);

    /**
     * @brief Tipo para o callback a ser executado após o tempo de espera.
     * Recebe um boost::system::error_code indicando o status da conclusão (sucesso ou cancelamento).
     */
    using Callback = std::function<void(const boost::system::error_code&)>;

    /**
     * @brief Inicia uma espera assíncrona.
     * 
     * @param duration Duração do tempo de espera (em milissegundos).
     * @param on_complete Função callback a ser chamada quando o tempo expirar.
     */
    void sleep_for(std::chrono::milliseconds duration, Callback on_complete);

private:
    boost::asio::steady_timer timer_; ///< Temporizador interno do Boost.Asio.
};

#endif // SLEEP_ASYNCH_H