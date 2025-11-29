#include "mine_generator.h"
#include <iostream>
#include <algorithm>
#include <random>

MineGenerator::MineGenerator(int width, int height) 
    : width(width), height(height), rng(std::random_device{}()) {
    
    // Garante dimensões ímpares para o algoritmo funcionar corretamente com paredes
    // O algoritmo de recursive backtracker precisa de células "impares" para criar caminhos e paredes alternados
    if (this->width % 2 == 0) this->width++;
    if (this->height % 2 == 0) this->height++;

    // Inicializa tudo como parede ('1')
    minefield.resize(this->height, std::vector<char>(this->width, '1'));
}

void MineGenerator::generate() {
    // 1. Gera o labirinto base (corredores estreitos)
    recursiveBacktracker(1, 1);

    // 2. Alarga os túneis (Dilation)
    // Transforma 20% das paredes adjacentes a caminhos em caminhos
    widenTunnels(0.2f);

    // 3. Adiciona salas de escavação (Rooms)
    // Adiciona 5 salas de 5x5
    addRooms(5);

    // 4. Define Start/End (garante que não foram sobrescritos)
    minefield[1][1] = 'A';
    minefield[lastY][lastX] = 'B';
}

void MineGenerator::addRooms(int numRooms) {
    std::uniform_int_distribution<int> distX(1, width - 6);
    std::uniform_int_distribution<int> distY(1, height - 6);

    for (int i = 0; i < numRooms; ++i) {
        int rx = distX(rng);
        int ry = distY(rng);

        // Cria uma sala 5x5
        for (int y = ry; y < ry + 5; ++y) {
            for (int x = rx; x < rx + 5; ++x) {
                if (y < height - 1 && x < width - 1) {
                    minefield[y][x] = '0';
                }
            }
        }
    }
}

void MineGenerator::widenTunnels(float probability) {
    std::vector<std::vector<char>> nextMap = minefield;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (minefield[y][x] == '1') {
                // Verifica se tem algum vizinho que é caminho ('0', 'A', 'B')
                bool hasPathNeighbor = false;
                int neighbors[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
                
                for (auto& n : neighbors) {
                    char c = minefield[y + n[1]][x + n[0]];
                    if (c == '0' || c == 'A' || c == 'B') {
                        hasPathNeighbor = true;
                        break;
                    }
                }

                if (hasPathNeighbor && dist(rng) < probability) {
                    nextMap[y][x] = '0';
                }
            }
        }
    }
    minefield = nextMap;
}

void MineGenerator::recursiveBacktracker(int x, int y) {
    minefield[y][x] = '0'; // Marca a célula atual como vazia (caminho visitado)
    
    // Atualiza a última posição visitada (potencial fim do labirinto)
    lastX = x;
    lastY = y;

    // Direções possíveis: Cima, Baixo, Esquerda, Direita
    // O passo é 2 para pular a parede intermediária
    int dirs[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}};
    
    // Embaralha as direções para garantir aleatoriedade na geração do labirinto
    std::shuffle(std::begin(dirs), std::end(dirs), rng);

    for (auto& dir : dirs) {
        int nx = x + dir[0];
        int ny = y + dir[1];

        // Verifica se o vizinho (pulando a parede) é válido e se ainda é uma parede (não visitado)
        if (isValid(nx, ny) && minefield[ny][nx] == '1') {
            // Remove a parede entre a célula atual e a próxima célula escolhida
            // A parede está na metade do caminho (dir[0]/2, dir[1]/2)
            minefield[y + dir[1]/2][x + dir[0]/2] = '0';
            
            // Chama recursivamente para a próxima célula
            recursiveBacktracker(nx, ny);
        }
    }
}

bool MineGenerator::isValid(int x, int y) {
    // Verifica se as coordenadas estão dentro dos limites do mapa (excluindo as bordas externas)
    return x > 0 && x < width - 1 && y > 0 && y < height - 1;
}

void MineGenerator::print() {
    for (const auto& row : minefield) {
        for (char cell : row) {
            // Imprime '##' para parede e '  ' para caminho vazio para melhor visualização
            if (cell == '1') std::cout << "##";
            else if (cell == '0') std::cout << "  ";
            else if (cell == 'A') std::cout << "AA";
            else if (cell == 'B') std::cout << "BB";
            else std::cout << "??";
        }
        std::cout << std::endl;
    }
}

const std::vector<std::vector<char>>& MineGenerator::getMinefield() const {
    return minefield;
}
