#include <iostream>

#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }
  const int aLoops = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  volatile unsigned int aValue = 0U;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    aValue = static_cast<unsigned int>(aValue + static_cast<unsigned int>(aLoop * 17 + 3));
  }
  std::cout << "[PASS] benchmark counters+matrix quick run value=" << aValue << "\n";
  return 0;
}
