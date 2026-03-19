#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/ExpanderDigestAdapter.hpp"
#include "tests/common/Tests.hpp"

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;
using peanutbutter::expansion::key_expansion::ByteTwister;

constexpr int kWindowSize = 64;
constexpr int kWindowCount = 32;
constexpr int kStreamLength = kWindowSize * kWindowCount;
constexpr int kMultiBlockCount = 3;
constexpr int kMultiBlockLength = PASSWORD_EXPANDED_SIZE * kMultiBlockCount;

const char* TypeName(PasswordExpander::Type pType) {
  switch (pType) {
    case PasswordExpander::kType00:
      return "ExpandPassword_00";
    case PasswordExpander::kType01:
      return "ExpandPassword_01";
    case PasswordExpander::kType02:
      return "ExpandPassword_02";
    case PasswordExpander::kType03:
      return "ExpandPassword_03";
    case PasswordExpander::kType04:
      return "ExpandPassword_04";
    case PasswordExpander::kType05:
      return "ExpandPassword_05";
    case PasswordExpander::kType06:
      return "ExpandPassword_06";
    case PasswordExpander::kType07:
      return "ExpandPassword_07";
    case PasswordExpander::kType08:
      return "ExpandPassword_08";
    case PasswordExpander::kType09:
      return "ExpandPassword_09";
    case PasswordExpander::kType10:
      return "ExpandPassword_10";
    case PasswordExpander::kType11:
      return "ExpandPassword_11";
    case PasswordExpander::kType12:
      return "ExpandPassword_12";
    case PasswordExpander::kType13:
      return "ExpandPassword_13";
    case PasswordExpander::kType14:
      return "ExpandPassword_14";
    case PasswordExpander::kType15:
      return "ExpandPassword_15";
    case PasswordExpander::kTypeCount:
      break;
  }
  return "ExpandPassword_Unknown";
}

bool AnyNonZero(const std::array<unsigned char, PASSWORD_EXPANDED_SIZE>& pBytes) {
  for (unsigned char aByte : pBytes) {
    if (aByte != 0U) {
      return true;
    }
  }
  return false;
}

bool HasRepeated64ByteWindow(const std::vector<unsigned char>& pBytes) {
  for (int aLeft = 0; aLeft < kWindowCount; ++aLeft) {
    const int aLeftOffset = aLeft * kWindowSize;
    for (int aRight = aLeft + 1; aRight < kWindowCount; ++aRight) {
      const int aRightOffset = aRight * kWindowSize;
      bool aEqual = true;
      for (int aIndex = 0; aIndex < kWindowSize; ++aIndex) {
        if (pBytes[static_cast<std::size_t>(aLeftOffset + aIndex)] !=
            pBytes[static_cast<std::size_t>(aRightOffset + aIndex)]) {
          aEqual = false;
          break;
        }
      }
      if (aEqual) {
        return true;
      }
    }
  }
  return false;
}

void RunManualTwisterChain(unsigned char pType,
                           unsigned char* pInitialSource,
                           unsigned char* pWorker,
                           unsigned char* pDestination,
                           int pBlockCount) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes] = {};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes] = {};
  unsigned char aSaltBuffer[twist::kSaltBytes] = {};

  ByteTwister::SeedKeyByIndex(pType, pInitialSource, aKeyBuffer, PASSWORD_EXPANDED_SIZE);
  ByteTwister::SeedSaltByIndex(pType, pInitialSource, aSaltBuffer, PASSWORD_EXPANDED_SIZE);
  for (int aBlockIndex = 0; aBlockIndex < pBlockCount; ++aBlockIndex) {
    unsigned char* aRoundSource =
        (aBlockIndex == 0) ? pInitialSource : (pDestination + ((aBlockIndex - 1) * PASSWORD_EXPANDED_SIZE));
    unsigned char* aRoundDestination = pDestination + (aBlockIndex * PASSWORD_EXPANDED_SIZE);
    ByteTwister::TwistBlockByIndex(
        pType,
        aRoundSource,
        pWorker,
        aRoundDestination,
        static_cast<unsigned int>(aBlockIndex),
        aSaltBuffer,
        aKeyBuffer,
        PASSWORD_EXPANDED_SIZE);
    ByteTwister::PushKeyRoundByIndex(
        pType, aRoundDestination, aSaltBuffer, aKeyBuffer, aNextRoundKeyBuffer, PASSWORD_EXPANDED_SIZE);
  }
}

}  // namespace

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aLengthCap =
      (peanutbutter::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 2166136261ULL;
  std::array<std::array<unsigned char, PASSWORD_EXPANDED_SIZE>, PasswordExpander::kTypeCount> aReferenceBlocks{};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    const int aLoopSeed = peanutbutter::tests::config::ApplyGlobalSeed(aLoop);
    const int aPasswordLength = (aLengthCap < 8) ? aLengthCap : 8;
    std::vector<unsigned char> aPassword(static_cast<std::size_t>(aPasswordLength), 0U);
    for (int aIndex = 0; aIndex < aPasswordLength; ++aIndex) {
      aPassword[static_cast<std::size_t>(aIndex)] =
          static_cast<unsigned char>((aLoopSeed * 31 + aIndex * 17 + 7) & 0xFF);
    }

    for (int aTypeIndex = 0; aTypeIndex < PasswordExpander::kTypeCount; ++aTypeIndex) {
      const auto aType = static_cast<PasswordExpander::Type>(aTypeIndex);
      std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpandedA = {};
      std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpandedB = {};

      PasswordExpander::ExpandPassword(aType, aPassword.data(), aWorker.data(), aExpandedA.data(),
                                       static_cast<unsigned int>(aPasswordLength));
      PasswordExpander::ExpandPassword(aType, aPassword.data(), aWorker.data(), aExpandedB.data(),
                                       static_cast<unsigned int>(aPasswordLength));

      std::vector<unsigned char> aExpandedLong(static_cast<std::size_t>(kMultiBlockLength), 0U);
      std::vector<unsigned char> aExpandedExpected(static_cast<std::size_t>(kMultiBlockLength), 0U);
      PasswordExpander::ExpandPasswordBlocks(aType,
                                             aPassword.data(),
                                             static_cast<unsigned int>(aPasswordLength),
                                             aWorker.data(),
                                             aExpandedLong.data(),
                                             static_cast<unsigned int>(kMultiBlockLength));
      std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpandedSeed = {};
      PasswordExpander::FillDoubledSource(
          aPassword.data(), static_cast<unsigned int>(aPasswordLength), aExpandedSeed.data());
      RunManualTwisterChain(
          static_cast<unsigned char>(aTypeIndex), aExpandedSeed.data(), aWorker.data(), aExpandedExpected.data(),
          kMultiBlockCount);

      if (aExpandedA != aExpandedB) {
        std::cerr << "[FAIL] " << TypeName(aType) << " output mismatch at loop " << aLoop << "\n";
        return 1;
      }
      if (aExpandedLong != aExpandedExpected) {
        std::cerr << "[FAIL] " << TypeName(aType)
                  << " multi-block expansion mismatch at loop " << aLoop << "\n";
        return 1;
      }
      if (!AnyNonZero(aExpandedA)) {
        std::cerr << "[FAIL] " << TypeName(aType) << " output unexpectedly zero at loop " << aLoop << "\n";
        return 1;
      }

      std::vector<unsigned char> aTwisterSource(static_cast<std::size_t>(kMultiBlockLength), 0U);
      for (int aIndex = 0; aIndex < kMultiBlockLength; ++aIndex) {
        aTwisterSource[static_cast<std::size_t>(aIndex)] =
            static_cast<unsigned char>((aLoopSeed + aTypeIndex * 37 + aIndex * 13 + (aIndex / PASSWORD_EXPANDED_SIZE)) & 0xFF);
      }

      std::vector<unsigned char> aTwistedLong(static_cast<std::size_t>(kMultiBlockLength), 0U);
      std::vector<unsigned char> aTwistedExpected(static_cast<std::size_t>(kMultiBlockLength), 0U);
      ByteTwister::TwistBytesByIndex(
          static_cast<unsigned char>(aTypeIndex),
          aTwisterSource.data(),
          aWorker.data(),
          aTwistedLong.data(),
          static_cast<unsigned int>(kMultiBlockLength));
      RunManualTwisterChain(
          static_cast<unsigned char>(aTypeIndex), aTwisterSource.data(), aWorker.data(), aTwistedExpected.data(),
          kMultiBlockCount);
      if (aTwistedLong != aTwistedExpected) {
        std::cerr << "[FAIL] " << TypeName(aType)
                  << " multi-block twister mismatch at loop " << aLoop << "\n";
        return 1;
      }

      ExpanderDigestAdapter aAdapter(aType);
      aAdapter.Seed(aPassword.data(), aPasswordLength);
      std::vector<unsigned char> aStream(static_cast<std::size_t>(kStreamLength), 0U);
      aAdapter.Get(aStream.data(), static_cast<int>(aStream.size()));
      if (HasRepeated64ByteWindow(aStream)) {
        std::cerr << "[FAIL] " << TypeName(aType)
                  << " repeated a 64-byte window within the first " << kStreamLength
                  << " bytes at loop " << aLoop << "\n";
        return 1;
      }

      if (aLoop == 0) {
        aReferenceBlocks[static_cast<std::size_t>(aTypeIndex)] = aExpandedA;
      }

      for (unsigned char aByte : aExpandedA) {
        aDigest = (aDigest * 16777619ULL) ^ static_cast<std::uint64_t>(aByte);
      }
      for (unsigned char aByte : aStream) {
        aDigest = (aDigest * 16777619ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
  }

  for (int aLeft = 0; aLeft < PasswordExpander::kTypeCount; ++aLeft) {
    for (int aRight = aLeft + 1; aRight < PasswordExpander::kTypeCount; ++aRight) {
      if (aReferenceBlocks[static_cast<std::size_t>(aLeft)] ==
          aReferenceBlocks[static_cast<std::size_t>(aRight)]) {
        std::cerr << "[FAIL] " << TypeName(static_cast<PasswordExpander::Type>(aLeft))
                  << " and " << TypeName(static_cast<PasswordExpander::Type>(aRight))
                  << " produced identical first blocks\n";
        return 1;
      }
    }
  }

  std::cout << "[PASS] password expander flows test passed"
            << " loops=" << aLoops
            << " types=" << PasswordExpander::kTypeCount
            << " digest=" << aDigest << "\n";
  return 0;
}
