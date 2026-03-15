#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"
#include "tests/common/Tests.hpp"

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aPasswordLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;

  bread::expansion::key_expansion::PasswordExpanderA aExpander;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpanded = {};
  std::uint64_t aDigest = 2166136261ULL;
  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    const int aLoopSeed = bread::tests::config::ApplyGlobalSeed(aLoop);
    std::vector<unsigned char> aPassword(static_cast<std::size_t>(aPasswordLength), 0U);
    for (int aIndex = 0; aIndex < aPasswordLength; ++aIndex) {
      aPassword[static_cast<std::size_t>(aIndex)] =
          static_cast<unsigned char>((aLoopSeed * 29 + aIndex * 13 + 5) & 0xFF);
    }
    aExpander.Expand(aPassword.data(), aPasswordLength, aExpanded.data());
    for (unsigned char aByte : aExpanded) {
      aDigest = (aDigest * 16777619ULL) ^ static_cast<std::uint64_t>(aByte);
    }
  }

  std::cout << "[PASS] benchmark password expander quick run"
            << " loops=" << aLoops << " bytes=" << aPasswordLength << " digest=" << aDigest << "\n";
  return 0;
}
