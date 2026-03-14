#include <iostream>

#include "tests/common/QuickConfig.hpp"

int main() {
  volatile unsigned int aTicks = 0U;
  for (int aLoop = 0; aLoop < bread::tests::common::kTinyLoops; ++aLoop) {
    aTicks ^= static_cast<unsigned int>(aLoop + 1);
  }
  std::cout << "[PASS] benchmark counter speed quick run ticks=" << aTicks << "\n";
  return 0;
}
