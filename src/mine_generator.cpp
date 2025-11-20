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
    // Começa em (1, 1) para garantir que haja bordas ao redor
    recursiveBacktracker(1, 1);

    // Define o ponto de início (A) e fim (B)
    minefield[1][1] = 'A';
    minefield[lastY][lastX] = 'B';
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
