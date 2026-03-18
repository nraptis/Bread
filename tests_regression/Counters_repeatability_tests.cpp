#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aOutputLength =
      (peanutbutter::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    const int aLoopSeed = peanutbutter::tests::config::ApplyGlobalSeed(aLoop);
    ChaCha20Counter aCounterA;
    ChaCha20Counter aCounterB;
    std::array<unsigned char, 8> aSeed = {{
        static_cast<unsigned char>(9U + static_cast<unsigned int>(aLoopSeed)),
        static_cast<unsigned char>(8U + static_cast<unsigned int>(aLoopSeed * 3)),
        static_cast<unsigned char>(7U + static_cast<unsigned int>(aLoopSeed * 5)),
        static_cast<unsigned char>(6U + static_cast<unsigned int>(aLoopSeed * 7)),
        static_cast<unsigned char>(5U + static_cast<unsigned int>(aLoopSeed * 11)),
        static_cast<unsigned char>(4U + static_cast<unsigned int>(aLoopSeed * 13)),
        static_cast<unsigned char>(3U + static_cast<unsigned int>(aLoopSeed * 17)),
        static_cast<unsigned char>(2U + static_cast<unsigned int>(aLoopSeed * 19)),
    }};
    std::vector<unsigned char> aOutA(static_cast<std::size_t>(aOutputLength), 0U);
    std::vector<unsigned char> aOutB(static_cast<std::size_t>(aOutputLength), 0U);

    aCounterA.Seed(aSeed.data(), static_cast<int>(aSeed.size()));
    aCounterB.Seed(aSeed.data(), static_cast<int>(aSeed.size()));
    aCounterA.Get(aOutA.data(), aOutputLength);
    aCounterB.Get(aOutB.data(), aOutputLength);

    if (aOutA != aOutB) {
      std::cerr << "[FAIL] counter repeatability mismatch at loop " << aLoop << "\n";
      return 1;
    }

    for (unsigned char aByte : aOutA) {
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
    }
  }

  std::cout << "[PASS] counter repeatability tests passed"
            << " loops=" << aLoops << " bytes=" << aOutputLength << " digest=" << aDigest << "\n";
  return 0;
}
