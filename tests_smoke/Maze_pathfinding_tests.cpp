#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/maze/MazePolicy.hpp"
#include "tests/common/CounterSeedBuffer.hpp"

namespace {

bool VerifyAllOpenTilesReachable(peanutbutter::maze::Maze* pMaze, int pStartX, int pStartY, const char* pName) {
  if (pMaze == nullptr) {
    std::cerr << "[FAIL] null maze in reachability check\n";
    return false;
  }
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
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

bool FindFirstOpenCell(peanutbutter::maze::Maze* pMaze, int* pOutX, int* pOutY) {
  if (pMaze == nullptr || pOutX == nullptr || pOutY == nullptr) {
    return false;
  }
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
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
    const std::vector<unsigned char> aExpandedSeed = peanutbutter::tests::BuildCounterSeedBuffer<ChaCha20Counter>(
        pSeed, pSeedLength, peanutbutter::maze::Maze::kSeedBufferCapacity);
    for (int aGameIndex = 0; aGameIndex < peanutbutter::maze::kGameCount; ++aGameIndex) {
      peanutbutter::maze::MazeDirector aMaze;
      aMaze.SetGame(aGameIndex);
      aMaze.Seed(const_cast<unsigned char*>(aExpandedSeed.data()), static_cast<int>(aExpandedSeed.size()));

      int aStartX = -1;
      int aStartY = -1;
      if (!FindFirstOpenCell(&aMaze, &aStartX, &aStartY)) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " has no open cell for seed " << pSeedName << "\n";
        return false;
      }

      int aEndX = -1;
      int aEndY = -1;
      if (!aMaze.GetRandomWalkable(aEndX, aEndY) || aMaze.IsWall(aEndX, aEndY)) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " random walkable lookup failed for seed " << pSeedName
                  << "\n";
        return false;
      }

      if (!aMaze.FindPath(aStartX, aStartY, aEndX, aEndY)) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " path was not found between walkable cells for seed "
                  << pSeedName << "\n";
        return false;
      }
      if (aMaze.PathLength() <= 0) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " path length is empty for seed " << pSeedName << "\n";
        return false;
      }

      if (!VerifyAllOpenTilesReachable(&aMaze, aStartX, aStartY, aMaze.GetName())) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " reachability failed for seed " << pSeedName << "\n";
        return false;
      }

      int aRandomX = -1;
      int aRandomY = -1;
      if (!aMaze.GetRandomWalkable(aRandomX, aRandomY) || aMaze.IsWall(aRandomX, aRandomY)) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " GetRandomWalkable failed for seed " << pSeedName << "\n";
        return false;
      }
      int aRandomWallX = -1;
      int aRandomWallY = -1;
      if (!aMaze.GetRandomWall(aRandomWallX, aRandomWallY) || !aMaze.IsWall(aRandomWallX, aRandomWallY)) {
        std::cerr << "[FAIL] " << aMaze.GetName() << " GetRandomWall failed for seed " << pSeedName << "\n";
        return false;
      }
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
