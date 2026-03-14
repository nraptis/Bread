#include <iostream>

#include "tests/common/QuickConfig.hpp"

int main() {
  volatile unsigned int aValue = 0U;
  for (int aLoop = 0; aLoop < bread::tests::common::kTinyLoops; ++aLoop) {
    aValue = static_cast<unsigned int>(aValue + static_cast<unsigned int>(aLoop * 17 + 3));
  }
  std::cout << "[PASS] benchmark counters+matrix quick run value=" << aValue << "\n";
  return 0;
}
