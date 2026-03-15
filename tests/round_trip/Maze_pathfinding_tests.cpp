#include <array>
#include <iostream>
#include <memory>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/MazeRobotCheese.hpp"

int main() {
  MersenneCounter aCounter;
  std::unique_ptr<bread::maze::Maze> aMaze = std::make_unique<bread::maze::MazeRobotCheese>(nullptr, &aCounter);

  std::array<unsigned char, 8> aSeed = {{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}};
  aMaze->Seed(aSeed.data(), static_cast<int>(aSeed.size()));

  if (!aMaze->FindPath(0, 0, bread::maze::Maze::kGridWidth - 1, bread::maze::Maze::kGridHeight - 1)) {
    std::cerr << "[FAIL] Maze A* path was not found\n";
    return 1;
  }
  if (aMaze->PathLength() <= 0) {
    std::cerr << "[FAIL] Maze path length is empty\n";
    return 1;
  }

  std::cout << "[PASS] maze pathfinding tests passed\n";
  return 0;
}
