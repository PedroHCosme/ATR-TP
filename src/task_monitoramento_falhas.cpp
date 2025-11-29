#include "task_monitoramento_falhas.h"
#include "interfaces/i_sensor_driver.h"
#include "utils/sleep_asynch.h" // Para timing preciso
#include <boost/asio.hpp>
#include <iostream>
#include <functional>

void task_monitoramento_falhas(GerenciadorDados& gerenciadorDados, EventosSistema& eventos, ISensorDriver& driver) {
    // 1. Setup do Motor de Tempo (Independente)
    boost::asio::io_context io;
    SleepAsynch timer(io);

    // Loop recursivo para garantir periodicidade
    std::function<void()> loop_monitoramento;

    loop_monitoramento = [&]() {
        // --- INÍCIO DA VERIFICAÇÃO DE SEGURANÇA ---

        // 1. Leitura Direta do Driver (Bypass do Buffer para Segurança)
        // Garante que estamos vendo o estado físico real, sem atrasos de filtro
        CaminhaoFisico estadoReal = driver.readSensorData(0); // Assumindo ID 0

        // Leitura do Buffer apenas para dados não-críticos ou redundância (opcional)
        // DadosSensores sensores = gerenciadorDados.lerUltimoEstado();

        bool nova_falha_detectada = false;
        int codigo_falha = 0;

        // 2. Verificação Termodinâmica
        // O PDF diz: Alerta > 95 (Apenas aviso), Defeito > 120 (Parada)
        
        // Debug: Print constante da temperatura monitorada
        std::cout << "[DEBUG-MONITOR] Temperatura Real (Driver): " << estadoReal.i_temperatura << " C" << std::endl;
        
        if (estadoReal.i_temperatura > 120) {
            std::cout << "[MONITOR] CRITICO: Superaquecimento do Motor (" 
                      << estadoReal.i_temperatura << " C)!" << std::endl;
            nova_falha_detectada = true;
            codigo_falha = 1; // Código 1: Temperatura
        }
        else if (estadoReal.i_temperatura > 95) {
            // Apenas um warning, não para o caminhão
            std::cout << "[MONITOR] ALERTA: Motor aquecendo (" << estadoReal.i_temperatura << " C)..." << std::endl;
        }

        // 3. Verificação de Subsistemas (Elétrica/Hidráulica)
        if (estadoReal.i_falha_eletrica) {
            std::cout << "[MONITOR] CRITICO: Falha Eletrica detectada!" << std::endl;
            nova_falha_detectada = true;
            codigo_falha = 2; // Código 2: Elétrica
        }

        if (estadoReal.i_falha_hidraulica) {
            std::cout << "[MONITOR] CRITICO: Falha Hidraulica detectada!" << std::endl;
            nova_falha_detectada = true;
            codigo_falha = 3; // Código 3: Hidráulica
        }

        // 4. Ação de Segurança (Interlock via Eventos)
        // Se detectou falha, sinaliza via EventosSistema.
        if (nova_falha_detectada) {
            if (!eventos.verificar_estado_falha()) { // Só sinaliza se ainda não estiver em falha
                eventos.sinalizar_falha(codigo_falha);
                std::cout << "[MONITOR] Falha sinalizada via EventosSistema (Codigo " << codigo_falha << ")." << std::endl;
            }
        }
        
        // (Opcional) Lógica de recuperação automática poderia vir aqui,
        // mas em sistemas críticos, geralmente o reset é manual (Comando de Rearme).

        // --- FIM DA LÓGICA ---

        // 5. Agendamento (5Hz é suficiente para monitoramento térmico)
        // 200ms de período
        timer.wait_next_tick(std::chrono::milliseconds(200), 
            [&](const boost::system::error_code& ec) {
                if (!ec) loop_monitoramento(); 
            }
        );
    };

    // Start
    loop_monitoramento();
    io.run();
}