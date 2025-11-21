#include "utils/sleep_asynch.h"

// Implementação do construtor
SleepAsynch::SleepAsynch(boost::asio::io_context& io) 
    : timer_(io) {
}

// Implementação do método
void SleepAsynch::sleep_for(std::chrono::milliseconds duration, Callback on_complete) {
    
    // Define o tempo de expiração
    timer_.expires_after(duration);
    
    // Inicia a espera assíncrona
    // Passamos o callback do usuário para o async_wait
    timer_.async_wait(on_complete);
}