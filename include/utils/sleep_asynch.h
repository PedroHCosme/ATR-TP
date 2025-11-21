#ifndef SLEEP_ASYNCH_H
#define SLEEP_ASYNCH_H

#include <chrono>
#include <functional> // Necessário para std::function
#include <boost/asio.hpp>

class SleepAsynch {
public:
    explicit SleepAsynch(boost::asio::io_context& io);

    // Definimos um tipo para o Callback para facilitar a leitura
    // Recebe um error_code (caso o timer seja cancelado)
    using Callback = std::function<void(const boost::system::error_code&)>;

    void sleep_for(std::chrono::milliseconds duration, Callback on_complete);

private:
    boost::asio::steady_timer timer_;
};

#endif // SLEEP_ASYNCH_H