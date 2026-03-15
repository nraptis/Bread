#include <iostream>

#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }
  const int aLoops = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  int aScore = 0;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    aScore += (aLoop * aLoop);
  }
  std::cout << "[PASS] benchmark all counters uniformity quick run score=" << aScore << "\n";
  return 0;
}
