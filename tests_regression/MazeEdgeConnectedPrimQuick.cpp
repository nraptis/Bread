#include <cstdint>
#include <iostream>
#include <vector>

#include "src/Tables/counters/MersenneCounter.hpp"
#include "tests/common/CounterSeedBuffer.hpp"
#include "tests/common/MazeEdgeConnectedUtils.hpp"
#include "tests/common/MazeGeneratorHarness.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] maze edge connected prim quick disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aSeedLength =
      (peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;

  std::cout << "[INFO] maze edge connected prim quick"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " global_seed=" << peanutbutter::tests::config::TEST_SEED_GLOBAL << "\n";

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
    peanutbutter::tests::maze_quality::FillInput(&aInput, aLoop, 0xD3A2646CU);
    const std::vector<unsigned char> aSeed =
        peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(aInput.data(), aSeedLength, aSeedLength);

    peanutbutter::tests::maze_generation::MazeGeneratorHarness aMaze;
    aMaze.Seed(const_cast<unsigned char*>(aSeed.data()), aSeedLength);
    aMaze.BuildPrims();

    if (!peanutbutter::tests::maze_quality::VerifyQuickConnectivity(&aMaze,
                                                             "maze edge connected prim quick",
                                                             aLoop,
                                                             0xD3A2646CU,
                                                             &aDigest)) {
      return 1;
    }
  }

  std::cout << "[PASS] maze edge connected prim quick passed"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " digest=" << aDigest << "\n";
  return 0;
}
