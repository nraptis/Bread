#include <array>
#include <iostream>

#include "src/counters/MersenneCounter.hpp"

int main() {
  MersenneCounter aCounterA;
  MersenneCounter aCounterB;
  std::array<unsigned char, 8> aSeed = {{9U, 8U, 7U, 6U, 5U, 4U, 3U, 2U}};
  std::array<unsigned char, 16> aOutA = {};
  std::array<unsigned char, 16> aOutB = {};

  aCounterA.Seed(aSeed.data(), static_cast<int>(aSeed.size()));
  aCounterB.Seed(aSeed.data(), static_cast<int>(aSeed.size()));
  aCounterA.Get(aOutA.data(), static_cast<int>(aOutA.size()));
  aCounterB.Get(aOutB.data(), static_cast<int>(aOutB.size()));

  if (aOutA != aOutB) {
    std::cerr << "[FAIL] counter repeatability mismatch\n";
    return 1;
  }

  std::cout << "[PASS] counter repeatability tests passed\n";
  return 0;
}
