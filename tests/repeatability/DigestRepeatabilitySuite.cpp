#include <iostream>

#include "tests/repeatability/DigestRepeatabilityJobs.hpp"

int main() {
  const bool aOk = bread::tests::repeatability::RunDigestRepeatabilityShort() &&
                   bread::tests::repeatability::RunDigestRepeatabilityMedium() &&
                   bread::tests::repeatability::RunDigestRepeatabilityOversized() &&
                   bread::tests::repeatability::RunDigestRepeatabilityContaminated() &&
                   bread::tests::repeatability::RunDigestRepeatabilityManual();
  if (!aOk) {
    std::cerr << "[FAIL] digest repeatability suite failed\n";
    return 1;
  }
  std::cout << "[PASS] digest repeatability suite passed\n";
  return 0;
}
