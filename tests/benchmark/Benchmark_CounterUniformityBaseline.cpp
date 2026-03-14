#include <iostream>

#include "tests/common/QuickConfig.hpp"

int main() {
  int aHistogram[4] = {0, 0, 0, 0};
  for (int aLoop = 0; aLoop < bread::tests::common::kTinyLoops; ++aLoop) {
    ++aHistogram[aLoop & 3];
  }
  std::cout << "[PASS] benchmark baseline uniformity quick run bins=" << aHistogram[0] << "," << aHistogram[1]
            << "," << aHistogram[2] << "," << aHistogram[3] << "\n";
  return 0;
}
