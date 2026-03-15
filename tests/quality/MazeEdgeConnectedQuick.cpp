#include <cstdint>
#include <iostream>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/MazeRobotCheese.hpp"
#include "tests/common/MazeEdgeConnectedUtils.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] maze edge connected quick disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aSeedLength =
      (bread::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::MAZE_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;

  std::cout << "[INFO] maze edge connected quick"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " global_seed=" << bread::tests::config::TEST_SEED_GLOBAL << "\n";

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
    bread::tests::maze_quality::FillInput(&aInput, aLoop, 0x85EBCA6BU);

    MersenneCounter aCounter;
    bread::maze::MazeRobotCheese aMaze(nullptr, &aCounter);
    aMaze.Seed(aInput.data(), aSeedLength);

    if (!bread::tests::maze_quality::VerifyQuickConnectivity(&aMaze,
                                                             "maze edge connected quick",
                                                             aLoop,
                                                             0x85EBCA6BU,
                                                             &aDigest)) {
      return 1;
    }
  }

  std::cout << "[PASS] maze edge connected quick passed"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " digest=" << aDigest << "\n";
  return 0;
}
