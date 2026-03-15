#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "tests/common/MazeEdgeConnectedUtils.hpp"
#include "tests/common/MazeGeneratorHarness.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] maze edge connected prim disabled by Tests.hpp\n";
    return 0;
  }

  constexpr int kLoopCap = 10;
  const int aLoopsRaw =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aLoops = (aLoopsRaw < kLoopCap) ? aLoopsRaw : kLoopCap;
  const int aSeedLength =
      (bread::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::MAZE_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;
  std::uint64_t aPairChecks = 0U;

  std::cout << "[INFO] maze edge connected prim"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " global_seed=" << bread::tests::config::TEST_SEED_GLOBAL << std::endl;

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
    bread::tests::maze_quality::FillInput(&aInput, aLoop, 0x165667B1U);

    bread::tests::maze_generation::MazeGeneratorHarness aMaze;
    aMaze.Seed(aInput.data(), aSeedLength);
    aMaze.BuildPrims();

    if (!bread::tests::maze_quality::VerifyExhaustiveConnectivity(&aMaze,
                                                                  "maze edge connected prim",
                                                                  aLoop,
                                                                  &aPairChecks,
                                                                  &aDigest)) {
      return 1;
    }

    std::vector<std::pair<int, int>> aOpenCells;
    bread::tests::maze_quality::CollectOpenCells(aMaze, &aOpenCells);

    if (((aLoop + 1) % 10) == 0 || (aLoop + 1) == aLoops) {
      std::cout << "[INFO] maze edge connected prim progress"
                << " loop=" << (aLoop + 1) << "/" << aLoops
                << " open_cells=" << aOpenCells.size()
                << " pair_checks=" << aPairChecks << std::endl;
    }
  }

  std::cout << "[PASS] maze edge connected prim passed"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " pair_checks=" << aPairChecks
            << " digest=" << aDigest << "\n";
  return 0;
}
