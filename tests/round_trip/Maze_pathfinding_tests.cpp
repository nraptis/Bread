#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/MazeDolphinSharks.hpp"
#include "src/maze/MazeRobotCheese.hpp"

namespace {

bool VerifyAllOpenTilesReachable(bread::maze::Maze* pMaze, int pStartX, int pStartY, const char* pName) {
  if (pMaze == nullptr) {
    std::cerr << "[FAIL] null maze in reachability check\n";
    return false;
  }
  for (int aY = 0; aY < bread::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < bread::maze::Maze::kGridWidth; ++aX) {
      if (pMaze->IsWall(aX, aY)) {
        continue;
      }
      if (!pMaze->FindPath(pStartX, pStartY, aX, aY)) {
        std::cerr << "[FAIL] " << pName << " has unreachable open tile at (" << aX << "," << aY << ")\n";
        return false;
      }
    }
  }
  return true;
}

bool FindFirstOpenCell(bread::maze::Maze* pMaze, int* pOutX, int* pOutY) {
  if (pMaze == nullptr || pOutX == nullptr || pOutY == nullptr) {
    return false;
  }
  for (int aY = 0; aY < bread::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < bread::maze::Maze::kGridWidth; ++aX) {
      if (!pMaze->IsWall(aX, aY)) {
        *pOutX = aX;
        *pOutY = aY;
        return true;
      }
    }
  }
  return false;
}

}  // namespace

int main() {
  const std::array<std::array<unsigned char, 8>, 1> aBinarySeeds = {{{{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}}}};
  const unsigned char aDisplaySeed[] = "display_robot_maze";

  auto RunSeed = [](const unsigned char* pSeed, int pSeedLength, const char* pSeedName) -> bool {
    MersenneCounter aRobotCounter;
    std::unique_ptr<bread::maze::Maze> aRobotBase = std::make_unique<bread::maze::MazeRobotCheese>(nullptr, &aRobotCounter);
    aRobotBase->Seed(const_cast<unsigned char*>(pSeed), pSeedLength);

    if (!aRobotBase->FindPath(0, 0, bread::maze::Maze::kGridWidth - 1, bread::maze::Maze::kGridHeight - 1)) {
      std::cerr << "[FAIL] Maze A* path was not found for seed " << pSeedName << "\n";
      return false;
    }
    if (aRobotBase->PathLength() <= 0) {
      std::cerr << "[FAIL] Maze path length is empty for seed " << pSeedName << "\n";
      return false;
    }

    auto* aRobotMaze = dynamic_cast<bread::maze::MazeRobotCheese*>(aRobotBase.get());
    if (aRobotMaze == nullptr) {
      std::cerr << "[FAIL] MazeRobotCheese cast failed for seed " << pSeedName << "\n";
      return false;
    }

    if (!VerifyAllOpenTilesReachable(aRobotBase.get(), aRobotMaze->RobotX(), aRobotMaze->RobotY(), "Robot maze")) {
      std::cerr << "[FAIL] Robot maze reachability failed for seed " << pSeedName << "\n";
      return false;
    }

    int aRandomX = -1;
    int aRandomY = -1;
    if (!aRobotBase->GetRandomWalkable(aRandomX, aRandomY) || aRobotBase->IsWall(aRandomX, aRandomY)) {
      std::cerr << "[FAIL] GetRandomWalkable failed for seed " << pSeedName << "\n";
      return false;
    }
    int aRandomWallX = -1;
    int aRandomWallY = -1;
    if (!aRobotBase->GetRandomWall(aRandomWallX, aRandomWallY) || !aRobotBase->IsWall(aRandomWallX, aRandomWallY)) {
      std::cerr << "[FAIL] GetRandomWall failed for seed " << pSeedName << "\n";
      return false;
    }

    MersenneCounter aDolphinCounter;
    std::unique_ptr<bread::maze::Maze> aDolphinMaze =
        std::make_unique<bread::maze::MazeDolphinSharks>(nullptr, &aDolphinCounter);
    aDolphinMaze->Seed(const_cast<unsigned char*>(pSeed), pSeedLength);
    int aOpenX = -1;
    int aOpenY = -1;
    if (!FindFirstOpenCell(aDolphinMaze.get(), &aOpenX, &aOpenY)) {
      std::cerr << "[FAIL] Dolphin maze has no open cell for seed " << pSeedName << "\n";
      return false;
    }
    if (!VerifyAllOpenTilesReachable(aDolphinMaze.get(), aOpenX, aOpenY, "Dolphin maze")) {
      std::cerr << "[FAIL] Dolphin maze reachability failed for seed " << pSeedName << "\n";
      return false;
    }
    return true;
  };

  for (const auto& aSeed : aBinarySeeds) {
    if (!RunSeed(aSeed.data(), static_cast<int>(aSeed.size()), "binary_seed")) {
      return 1;
    }
  }
  if (!RunSeed(aDisplaySeed, static_cast<int>(sizeof(aDisplaySeed) - 1), "display_robot_maze")) {
    return 1;
  }

  std::cout << "[PASS] maze pathfinding tests passed\n";
  return 0;
}
