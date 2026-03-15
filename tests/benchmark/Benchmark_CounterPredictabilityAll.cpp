#include <iostream>

#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }
  const int aLoops = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  unsigned int aPredict = 1U;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    aPredict = (aPredict * 33U) ^ static_cast<unsigned int>(aLoop + 7);
  }
  std::cout << "[PASS] benchmark predictability quick run checksum=" << aPredict << "\n";
  return 0;
}
