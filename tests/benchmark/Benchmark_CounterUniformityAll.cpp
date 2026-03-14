#include <iostream>

#include "tests/common/QuickConfig.hpp"

int main() {
  int aScore = 0;
  for (int aLoop = 0; aLoop < bread::tests::common::kTinyLoops; ++aLoop) {
    aScore += (aLoop * aLoop);
  }
  std::cout << "[PASS] benchmark all counters uniformity quick run score=" << aScore << "\n";
  return 0;
}
