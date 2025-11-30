#include "mine_generator.h"
#include <algorithm>
#include <iostream>
#include <random>

MineGenerator::MineGenerator(int width, int height)
    : width(width), height(height), rng(std::random_device{}()) {

  // Garante dimensões ímpares para o algoritmo funcionar corretamente com
  // paredes O algoritmo de recursive backtracker precisa de células "impares"
  // para criar caminhos e paredes alternados
  if (this->width % 2 == 0)
    this->width++;
  if (this->height % 2 == 0)
    this->height++;

  // Inicializa tudo como parede ('1')
  minefield.resize(this->height, std::vector<char>(this->width, '1'));
}

void MineGenerator::generate() {
  // 1. Gera o labirinto base (corredores estreitos)
  recursiveBacktracker(1, 1);

  // 2. Alarga os túneis (Dilation)
  // Aumentado para 80% para criar corredores MUITO largos
  widenTunnels(0.8f);
  // Segunda passada para garantir que não haja gargalos
  widenTunnels(0.3f);

  // 3. Adiciona salas de escavação (Rooms)
  // Salas maiores (10x10) e mais numerosas (8)
  addRooms(8);

  // 4. Cria zona segura de 15x15 no início
  createStartRoom();

  // 5. Define Start/End
  // Start no centro da sala 15x15 (8,8)
  minefield[8][8] = 'A';
  minefield[lastY][lastX] = 'B';
}

void MineGenerator::createStartRoom() {
  // Garante uma área livre de 15x15 a partir de (1,1)
  // Isso cria um "hub" central grande para o início
  for (int y = 1; y <= 15 && y < height - 1; ++y) {
    for (int x = 1; x <= 15 && x < width - 1; ++x) {
      minefield[y][x] = '0';
    }
  }
}

void MineGenerator::addRooms(int numRooms) {
  std::uniform_int_distribution<int> distX(1, width - 11);
  std::uniform_int_distribution<int> distY(1, height - 11);

  for (int i = 0; i < numRooms; ++i) {
    int rx = distX(rng);
    int ry = distY(rng);

    // Cria uma sala 10x10 (antes era 5x5)
    for (int y = ry; y < ry + 10; ++y) {
      for (int x = rx; x < rx + 10; ++x) {
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

        for (auto &n : neighbors) {
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

  for (auto &dir : dirs) {
    int nx = x + dir[0];
    int ny = y + dir[1];

    // Verifica se o vizinho (pulando a parede) é válido e se ainda é uma parede
    // (não visitado)
    if (isValid(nx, ny) && minefield[ny][nx] == '1') {
      // Remove a parede entre a célula atual e a próxima célula escolhida
      // A parede está na metade do caminho (dir[0]/2, dir[1]/2)
      minefield[y + dir[1] / 2][x + dir[0] / 2] = '0';

      // Chama recursivamente para a próxima célula
      recursiveBacktracker(nx, ny);
    }
  }
}

bool MineGenerator::isValid(int x, int y) {
  // Verifica se as coordenadas estão dentro dos limites do mapa (excluindo as
  // bordas externas)
  return x > 0 && x < width - 1 && y > 0 && y < height - 1;
}

void MineGenerator::print() {
  for (const auto &row : minefield) {
    for (char cell : row) {
      // Imprime '##' para parede e '  ' para caminho vazio para melhor
      // visualização
      if (cell == '1') {
        // std::cout << "##";
      } else if (cell == '0') {
        // std::cout << "  ";
      } else if (cell == 'A') {
        // std::cout << "AA";
      } else if (cell == 'B') {
        // std::cout << "BB";
      } else {
        // std::cout << "??";
      }
    }
    // std::cout << std::endl;
  }
}

const std::vector<std::vector<char>> &MineGenerator::getMinefield() const {
  return minefield;
}
