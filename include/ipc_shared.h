#ifndef IPC_SHARED_H
#define IPC_SHARED_H

#include "dados.h"
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <string>

// Base Port for Truck 0. Truck 1 = 9001, Truck 2 = 9002.
const int BASE_PORT = 9100;

/**
 * @struct ServerToClientPacket
 * @brief Pacote enviado do Servidor (Coletor) para o Cliente (Cockpit).
 */
struct ServerToClientPacket {
  DadosSensores sensores;
  EstadoVeiculo estado;
};

/**
 * @struct ClientToServerPacket
 * @brief Pacote enviado do Cliente (Cockpit) para o Servidor (Coletor).
 */
struct ClientToServerPacket {
  ComandosOperador comandos;
};

#endif // IPC_SHARED_H
