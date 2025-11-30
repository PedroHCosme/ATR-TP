#ifndef MINE_GENERATOR_H
#define MINE_GENERATOR_H

#include <random>
#include <vector>

/**
 * @class MineGenerator
 * @brief Gera um mapa de mina (labirinto) usando o algoritmo Recursive
 * Backtracker.
 *
 * O mapa é representado por uma grid onde:
 * 0 = Espaço vazio (caminho)
 * 1 = Parede (rocha)
 */
class MineGenerator {
public:
  /**
   * @brief Construtor.
   * @param width Largura do mapa (será ajustada para ímpar se necessário).
   * @param height Altura do mapa (será ajustada para ímpar se necessário).
   */
  MineGenerator(int width, int height);

  /**
   * @brief Executa o algoritmo de geração da mina.
   */
  void generate();

  /**
   * @brief Adiciona salas de escavação (áreas abertas) ao mapa.
   * @param numRooms Número de salas a serem criadas.
   */
  void addRooms(int numRooms);

  /**
   * @brief Alarga os túneis transformando paredes aleatórias em caminhos.
   * @param probability Probabilidade (0.0 a 1.0) de uma parede adjacente a um
   * caminho ser removida.
   */
  void widenTunnels(float probability);

  /**
   * @brief Imprime o mapa no console para visualização.
   */
  void print();

  /**
   * @brief Retorna o mapa gerado.
   * @return Referência constante para a matriz do mapa.
   */
  const std::vector<std::vector<char>> &getMinefield() const;

private:
  int width;
  int height;
  std::vector<std::vector<char>>
      minefield; // '0': Vazio, '1': Parede, 'A': Inicio, 'B': Fim
  std::mt19937 rng;
  int lastX, lastY; // Armazena a última posição gerada para colocar o 'B'

  /**
   * @brief Função recursiva principal do algoritmo.
   * @param x Coordenada X atual.
   * @param y Coordenada Y atual.
   */
  void recursiveBacktracker(int x, int y);

  /**
   * @brief Verifica se as coordenadas estão dentro dos limites seguros do mapa.
   */
  bool isValid(int x, int y);
  void createStartRoom();
};

#endif // MINE_GENERATOR_H