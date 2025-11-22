#include "utils/sleep_asynch.h"

// Implementação do construtor
SleepAsynch::SleepAsynch(boost::asio::io_context& io) 
    : timer_(io) {
        
     next_wake_time_ = std::chrono::steady_clock::now();
}

// Implementação do método
void SleepAsynch::wait_next_tick(std::chrono::milliseconds period, Callback on_complete) {
    
    // Define o tempo de expiração
    next_wake_time_ += period;
    timer_.expires_at(next_wake_time_); 
    timer_.async_wait(on_complete);
}