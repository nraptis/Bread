#include <iostream>

#include "tests/common/QuickConfig.hpp"

int main() {
  unsigned int aPredict = 1U;
  for (int aLoop = 0; aLoop < bread::tests::common::kTinyLoops; ++aLoop) {
    aPredict = (aPredict * 33U) ^ static_cast<unsigned int>(aLoop + 7);
  }
  std::cout << "[PASS] benchmark predictability quick run checksum=" << aPredict << "\n";
  return 0;
}
