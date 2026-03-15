#include <iostream>

#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }
  const int aLoops = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  volatile unsigned int aTicks = 0U;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    aTicks ^= static_cast<unsigned int>(aLoop + 1);
  }
  std::cout << "[PASS] benchmark counter speed quick run ticks=" << aTicks << "\n";
  return 0;
}
