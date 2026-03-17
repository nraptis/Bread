#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "src/Tables/fast_rand/FastRand.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/Tests.hpp"

namespace {

unsigned char ComputeExpectedTwistType(const unsigned char* pBuffer) {
  if (pBuffer == nullptr) {
    return 0U;
  }

  const unsigned int aFirst = static_cast<unsigned int>(pBuffer[0]);
  const unsigned int aLast = static_cast<unsigned int>(pBuffer[PASSWORD_EXPANDED_SIZE - 1]);
  const unsigned int aMagicIndex = ((aFirst << 8U) | aLast) % static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE);
  const unsigned char aMagicByte = pBuffer[aMagicIndex];
  return static_cast<unsigned char>(aMagicByte & 15U);
}

}  // namespace

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  constexpr int kProbeBytes = (PASSWORD_EXPANDED_SIZE * 2) + 1;
  std::uint64_t aDigest = 1469598103934665603ULL;

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    const int aLoopSeed = peanutbutter::tests::config::ApplyGlobalSeed(aLoop);
    std::array<unsigned char, 8> aPassword = {{
        static_cast<unsigned char>(0x11U + static_cast<unsigned int>(aLoopSeed)),
        static_cast<unsigned char>(0x22U + static_cast<unsigned int>(aLoopSeed * 3)),
        static_cast<unsigned char>(0x33U + static_cast<unsigned int>(aLoopSeed * 5)),
        static_cast<unsigned char>(0x44U + static_cast<unsigned int>(aLoopSeed * 7)),
        static_cast<unsigned char>(0x55U + static_cast<unsigned int>(aLoopSeed * 11)),
        static_cast<unsigned char>(0x66U + static_cast<unsigned int>(aLoopSeed * 13)),
        static_cast<unsigned char>(0x77U + static_cast<unsigned int>(aLoopSeed * 17)),
        static_cast<unsigned char>(0x88U + static_cast<unsigned int>(aLoopSeed * 19)),
    }};

    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aRepeatedSeed = {};
    peanutbutter::expansion::key_expansion::PasswordExpander::FillDoubledSource(
        aPassword.data(), static_cast<unsigned int>(aPassword.size()), aRepeatedSeed.data());
    const unsigned char aInitialExpectedType = ComputeExpectedTwistType(aRepeatedSeed.data());

    peanutbutter::fast_rand::FastRand aRandA;
    peanutbutter::fast_rand::FastRand aRandB;
    aRandA.Seed(aPassword.data(), static_cast<int>(aPassword.size()));
    aRandB.Seed(aPassword.data(), static_cast<int>(aPassword.size()));

    if (aRandA.CurrentTwistType() != aInitialExpectedType || aRandB.CurrentTwistType() != aInitialExpectedType) {
      std::cerr << "[FAIL] initial twist type mismatch at loop " << aLoop << "\n";
      return 1;
    }

    std::vector<unsigned char> aOutA(static_cast<std::size_t>(kProbeBytes), 0U);
    std::vector<unsigned char> aOutB(static_cast<std::size_t>(kProbeBytes), 0U);

    for (int aIndex = 0; aIndex < kProbeBytes; ++aIndex) {
      aOutA[static_cast<std::size_t>(aIndex)] = aRandA.Get();
      aOutB[static_cast<std::size_t>(aIndex)] = aRandB.Get();

      if (aOutA[static_cast<std::size_t>(aIndex)] != aOutB[static_cast<std::size_t>(aIndex)]) {
        std::cerr << "[FAIL] FastRand output mismatch at loop " << aLoop
                  << " byte=" << aIndex << "\n";
        return 1;
      }

      if (aIndex == PASSWORD_EXPANDED_SIZE) {
        const unsigned char aExpectedType = ComputeExpectedTwistType(aOutA.data());
        if (aRandA.CurrentTwistType() != aExpectedType || aRandB.CurrentTwistType() != aExpectedType) {
          std::cerr << "[FAIL] first wrap twist type mismatch at loop " << aLoop << "\n";
          return 1;
        }
      }
      if (aIndex == (PASSWORD_EXPANDED_SIZE * 2)) {
        const unsigned char aExpectedType =
            ComputeExpectedTwistType(aOutA.data() + PASSWORD_EXPANDED_SIZE);
        if (aRandA.CurrentTwistType() != aExpectedType || aRandB.CurrentTwistType() != aExpectedType) {
          std::cerr << "[FAIL] second wrap twist type mismatch at loop " << aLoop << "\n";
          return 1;
        }
      }
    }

    for (unsigned char aByte : aOutA) {
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
    }
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aRandA.CurrentTwistType());
  }

  std::cout << "[PASS] FastRand repeatability tests passed"
            << " loops=" << aLoops
            << " bytes=" << kProbeBytes
            << " digest=" << aDigest << "\n";
  return 0;
}
