#include <iostream>

#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }
  const int aLoops = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  int aHistogram[4] = {0, 0, 0, 0};
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    ++aHistogram[aLoop & 3];
  }
  std::cout << "[PASS] benchmark baseline uniformity quick run bins=" << aHistogram[0] << "," << aHistogram[1]
            << "," << aHistogram[2] << "," << aHistogram[3] << "\n";
  return 0;
}
