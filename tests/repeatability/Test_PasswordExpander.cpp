#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aLengthCap =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;

  bread::expansion::key_expansion::PasswordExpanderA aExpander;
  std::uint64_t aDigest = 2166136261ULL;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    const int aLoopSeed = bread::tests::config::ApplyGlobalSeed(aLoop);
    const int aPasswordLength = (aLengthCap < 4) ? aLengthCap : 4;
    std::vector<unsigned char> aPassword(static_cast<std::size_t>(aPasswordLength), 0U);
    for (int aIndex = 0; aIndex < aPasswordLength; ++aIndex) {
      aPassword[static_cast<std::size_t>(aIndex)] =
          static_cast<unsigned char>((aLoopSeed * 31 + aIndex * 17 + 7) & 0xFF);
    }

    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpandedA = {};
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpandedB = {};
    aExpander.Expand(aPassword.data(), aPasswordLength, aExpandedA.data());
    aExpander.Expand(aPassword.data(), aPasswordLength, aExpandedB.data());

    if (aExpandedA != aExpandedB) {
      std::cerr << "[FAIL] password expander output mismatch at loop " << aLoop << "\n";
      return 1;
    }

    int aNonZero = 0;
    for (int aIndex = 0; aIndex < 16; ++aIndex) {
      if (aExpandedA[static_cast<std::size_t>(aIndex)] != 0U) {
        ++aNonZero;
      }
    }
    if (aNonZero == 0) {
      std::cerr << "[FAIL] password expander output unexpectedly zero at loop " << aLoop << "\n";
      return 1;
    }

    for (unsigned char aByte : aExpandedA) {
      aDigest = (aDigest * 16777619ULL) ^ static_cast<std::uint64_t>(aByte);
    }
  }

  std::cout << "[PASS] password expander test passed loops=" << aLoops << " digest=" << aDigest << "\n";
  return 0;
}
