#ifndef SERVER_IPC_H
#define SERVER_IPC_H

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <netinet/in.h>
#include "gerenciador_dados.h"
#include "simulacao_mina.h"
#include "eventos_sistema.h"

class ServerIPC {
private:
    int server_fd;
    int client_fd;
    std::atomic<bool> running;
    std::thread net_thread;
    
    GerenciadorDados& dados;
    SimulacaoMina& simulacao;
    EventosSistema& eventos;
    const std::vector<std::vector<char>>& mapa;

public:
    ServerIPC(GerenciadorDados& d, SimulacaoMina& s, EventosSistema& e, const std::vector<std::vector<char>>& m);
    ~ServerIPC();

    void start();
    void stop();

private:
    void loop();
    void handle_client();
    void send_state();
    void process_command(const std::string& json);
};

#endif // SERVER_IPC_H
