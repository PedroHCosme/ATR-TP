#include <boost/asio.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>

// Includes do Sistema
#include "drivers/mqtt_driver.h" // Driver MQTT
#include "eventos_sistema.h"
#include "gerenciador_dados.h"
#include "interface_caminhao.h" // Cockpit Integrado
#include "utils/sleep_asynch.h"

// Includes das Tasks
#include "task_coletor_dados.h"
#include "task_collision_avoidance.h"
#include "task_controle_navegacao.h"
#include "task_logica_comando.h"
#include "task_monitoramento_falhas.h"
#include "task_planejamento_rota.h"
#include "task_tratamento_sensores.h"

/**
 * @brief Função principal do sistema de controle (Embarcado).
 */
int main(int argc, char *argv[]) {
  // Parse Truck ID
  int truck_id = 0;
  if (argc > 1) {
    try {
      truck_id = std::stoi(argv[1]);
    } catch (...) {
      std::cerr << "Invalid Truck ID argument. Defaulting to 0." << std::endl;
    }
  }
  std::cout << "[Main] Starting Controller for Truck ID: " << truck_id
            << std::endl;

  // Redireciona logs para arquivo (NECESSARIO PARA COCKPIT EMBARCADO)
  std::ofstream logfile("controlador.log");
  std::streambuf *cout_backup = std::cout.rdbuf();
  std::cout.rdbuf(logfile.rdbuf());
  std::streambuf *cerr_backup = std::cerr.rdbuf();
  std::cerr.rdbuf(logfile.rdbuf());

  // --- 1. SETUP INICIAL ---
  boost::asio::io_context io;

  // --- 2. INSTANCIAÇÃO DOS OBJETOS ---
  // 1. Instancia Objetos Compartilhados
  GerenciadorDados gerenciadorDados;
  EventosSistema eventos;

  // 2. Instancia Drivers
  // Driver MQTT conecta ao broker local (Mosquitto)
  // O Simulador Headless estará rodando em outro processo e publicando dados
  MqttDriver mqtt_driver("127.0.0.1", 1883, truck_id);

  // --- 3. CONFIGURAÇÃO DE ESTADO INICIAL ---
  EstadoVeiculo estadoInicial = {false, true}; // Modo Automático
  gerenciadorDados.setEstadoVeiculo(estadoInicial);

  ComandosOperador comandosIniciais = {true,  false, false,
                                       false, false, false}; // Solicita Auto
  gerenciadorDados.setComandosOperador(comandosIniciais);

  // --- 4. THREADS DO SISTEMA EMBARCADO ---

  // Thread 1: Tratamento de Sensores (Lê do MQTT)
  std::thread t1(task_tratamento_sensores, std::ref(gerenciadorDados),
                 std::ref(mqtt_driver), 0);

  // Thread 2: Lógica de Comando
  std::thread t2(task_logica_comando, std::ref(gerenciadorDados),
                 std::ref(eventos));

  // Thread 3: Controle de Navegação
  std::thread t_navegacao(task_controle_navegacao, std::ref(gerenciadorDados),
                          std::ref(eventos));

  std::thread t_coletor(task_coletor_dados, std::ref(gerenciadorDados),
                        std::ref(eventos), truck_id);

  // 4. Aguardar as threads (embora elas rodem indefinidamente)o de Falhas (Atua
  // via MQTT se precisar parar)
  std::thread t3(task_monitoramento_falhas, std::ref(gerenciadorDados),
                 std::ref(eventos), std::ref(mqtt_driver));

  // Thread 5: Collision Avoidance System (CAS) - Alta Prioridade
  std::thread t_cas(task_collision_avoidance, std::ref(gerenciadorDados),
                    std::ref(eventos), std::ref(mqtt_driver));

  // Thread 6: Planejamento de Rota (O General)
  std::thread t_rota(task_planejamento_rota, std::ref(gerenciadorDados),
                     std::ref(mqtt_driver));

  // Thread 5: Loop de Atuação (Envia comandos de controle para o MQTT)
  // Precisamos de uma task que pegue o comando do GerenciadorDados e envie para
  // o Driver Antes isso era feito no loop de simulação. Agora precisamos de um
  // loop dedicado de atuação.
  std::thread t_atuacao([&gerenciadorDados, &mqtt_driver, &eventos]() {
    while (true) {
      ComandosAtuador cmd = gerenciadorDados.getComandosAtuador();
      mqtt_driver.setAtuadores(cmd.aceleracao, cmd.direcao);

      // Sincroniza estado do sistema com o simulador/interface
      EstadoVeiculo estado = gerenciadorDados.getEstadoVeiculo();
      bool isFault = eventos.verificar_estado_falha();
      mqtt_driver.publishSystemState(estado.e_automatico == false, isFault);

      std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10Hz
    }
  });

  // --- 5. INTERFACE LOCAL (COCKPIT) ---
  // Cockpit Reintegrado (Direct Buffer Access)
  std::cout << "[Main] Iniciando Cockpit Integrado..." << std::endl;
  InterfaceCaminhao interface(truck_id, gerenciadorDados);
  std::thread t_ui(&InterfaceCaminhao::run, &interface);

  // Mantém a thread principal viva
  t_ui.join(); // Wait for UI to close (user presses Q)

  // --- 6. ENCERRAMENTO ---
  std::cout.rdbuf(cout_backup);
  std::cerr.rdbuf(cerr_backup);

  t1.detach();
  t2.detach();
  t3.detach();
  t_cas.detach();
  t_rota.detach();
  t_navegacao.detach();
  t_coletor.detach();
  t_atuacao.detach();

  return 0;
}