#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "src/Tables/maze/MazeGrid.hpp"

namespace {

class MazeGridValidityHarness final : public peanutbutter::maze::MazeGrid {
 public:
  void Seed(std::uint32_t pSeed) {
    mIndexState = (pSeed == 0U) ? 0x811C9DC5U : pSeed;
  }

  void BuildCustom() {
    GenerateCustom();
    FinalizeWalls();
  }

  void BuildPrims() {
    GeneratePrims();
    FinalizeWalls();
  }

  void BuildKruskals() {
    ExecuteKruskals();
    FinalizeWalls();
  }

  using peanutbutter::maze::MazeGrid::FindIslands;
  using peanutbutter::maze::MazeGrid::GetRandomWalkable;
  using peanutbutter::maze::MazeGrid::GroupCount;
  using peanutbutter::maze::MazeGrid::IsWall;

 protected:
  int NextIndex(int pLimit) override {
    if (pLimit <= 1) {
      return 0;
    }

    mIndexState ^= (mIndexState << 13U);
    mIndexState ^= (mIndexState >> 17U);
    mIndexState ^= (mIndexState << 5U);
    return static_cast<int>(mIndexState % static_cast<std::uint32_t>(pLimit));
  }

 private:
  std::uint32_t mIndexState = 0x811C9DC5U;
};

using BuildFn = void (MazeGridValidityHarness::*)();

std::uint32_t SeedForTrial(int pModeIndex, int pTrial) {
  return 0x9E3779B9U ^ (static_cast<std::uint32_t>(pModeIndex + 1) * 0x85EBCA6BU) ^
         (static_cast<std::uint32_t>(pTrial + 1) * 0xC2B2AE35U);
}

bool ValidateMode(const char* pModeName, int pModeIndex, BuildFn pBuildFn, int pTrials) {
  MazeGridValidityHarness aMaze;
  for (int aTrial = 0; aTrial < pTrials; ++aTrial) {
    aMaze.Seed(SeedForTrial(pModeIndex, aTrial));
    (aMaze.*pBuildFn)();
    aMaze.FindIslands();

    if (aMaze.GroupCount() != 1) {
      std::cerr << "[FAIL] mode=" << pModeName
                << " trial=" << aTrial
                << " group_count=" << aMaze.GroupCount() << "\n";
      return false;
    }

    int aX = -1;
    int aY = -1;
    if (!aMaze.GetRandomWalkable(aX, aY) || aMaze.IsWall(aX, aY)) {
      std::cerr << "[FAIL] mode=" << pModeName
                << " trial=" << aTrial
                << " reason=no_walkable_tile\n";
      return false;
    }
  }

  std::cout << "[PASS] mode=" << pModeName << " trials=" << pTrials << "\n";
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  int aTrials = 100;
  if (argc > 1) {
    aTrials = std::atoi(argv[1]);
  }
  if (aTrials <= 0) {
    std::cerr << "[FAIL] invalid trial count\n";
    return 1;
  }

  std::cout << "[INFO] maze grid validity trials_per_mode=" << aTrials << "\n";

  if (!ValidateMode("custom", 0, &MazeGridValidityHarness::BuildCustom, aTrials)) {
    return 1;
  }
  if (!ValidateMode("prim", 1, &MazeGridValidityHarness::BuildPrims, aTrials)) {
    return 1;
  }
  if (!ValidateMode("kruskal", 2, &MazeGridValidityHarness::BuildKruskals, aTrials)) {
    return 1;
  }

  std::cout << "[PASS] maze grid validity tests passed\n";
  return 0;
}
