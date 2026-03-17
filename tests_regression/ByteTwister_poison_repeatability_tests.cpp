#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "src/Tables/password_expanders/ByteTwister.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/Tests.hpp"

namespace {

using peanutbutter::expansion::key_expansion::ByteTwister;
using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr int kMultiBlockCount = 3;
constexpr int kMultiBlockLength = PASSWORD_EXPANDED_SIZE * kMultiBlockCount;

std::uint32_t NextState(std::uint32_t pState) {
  std::uint32_t aState = pState;
  aState ^= (aState << 13U);
  aState ^= (aState >> 17U);
  aState ^= (aState << 5U);
  return aState;
}

void FillPattern(unsigned char* pBuffer, std::size_t pLength, std::uint32_t pSeed) {
  if (pBuffer == nullptr) {
    return;
  }

  std::uint32_t aState = pSeed;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aState = NextState(aState + static_cast<std::uint32_t>(aIndex + 1U));
    pBuffer[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

std::uint64_t HashBytes(const unsigned char* pBuffer, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBuffer[aIndex]);
  }
  return aDigest;
}

std::vector<unsigned char> MakePassword(int pLoop, int pLength) {
  std::vector<unsigned char> aPassword(static_cast<std::size_t>(pLength), 0U);
  std::uint32_t aState =
      0xA53C9E11U ^ static_cast<std::uint32_t>(peanutbutter::tests::config::ApplyGlobalSeed(pLoop + pLength));
  for (int aIndex = 0; aIndex < pLength; ++aIndex) {
    aState = NextState(aState + static_cast<std::uint32_t>((aIndex * 17) + 3));
    aPassword[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>(aState & 0xFFU);
  }
  return aPassword;
}

bool CheckEqual(const unsigned char* pLeft,
                const unsigned char* pRight,
                std::size_t pLength,
                const char* pCaseName,
                int pLoop,
                int pTypeIndex,
                int pPasswordLength) {
  if (std::memcmp(pLeft, pRight, pLength) == 0) {
    return true;
  }

  std::cerr << "[FAIL] " << pCaseName
            << " mismatch loop=" << pLoop
            << " type=" << pTypeIndex
            << " password_length=" << pPasswordLength << "\n";
  return false;
}

}  // namespace

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] poisoned repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const std::array<int, 7> aPasswordLengths = {{1, 2, 7, 31, 64, 255, PASSWORD_EXPANDED_SIZE}};

  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aSourceA = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aSourceB = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorkerA = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorkerB = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aDestinationA = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aDestinationB = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aAliasA = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aAliasB = {};
  std::vector<unsigned char> aLongA(static_cast<std::size_t>(kMultiBlockLength), 0U);
  std::vector<unsigned char> aLongB(static_cast<std::size_t>(kMultiBlockLength), 0U);
  std::uint64_t aDigest = 1469598103934665603ULL;

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    for (int aPasswordLength : aPasswordLengths) {
      std::vector<unsigned char> aPassword = MakePassword(aLoop, aPasswordLength);

      for (int aTypeIndex = 0; aTypeIndex < ByteTwister::kTypeCount; ++aTypeIndex) {
        const unsigned char aType = static_cast<unsigned char>(aTypeIndex);

        FillPattern(aWorkerA.data(), aWorkerA.size(), 0x11110000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aWorkerB.data(), aWorkerB.size(), 0x22220000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aDestinationA.data(), aDestinationA.size(), 0x33330000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aDestinationB.data(), aDestinationB.size(), 0x44440000U ^ static_cast<std::uint32_t>(aTypeIndex));

        PasswordExpander::ExpandPasswordByIndex(
            aType, aPassword.data(), aWorkerA.data(), aDestinationA.data(), static_cast<unsigned int>(aPasswordLength));
        PasswordExpander::ExpandPasswordByIndex(
            aType, aPassword.data(), aWorkerB.data(), aDestinationB.data(), static_cast<unsigned int>(aPasswordLength));

        if (!CheckEqual(aDestinationA.data(),
                        aDestinationB.data(),
                        aDestinationA.size(),
                        "PasswordExpander external worker",
                        aLoop,
                        aTypeIndex,
                        aPasswordLength)) {
          return 1;
        }

        FillPattern(aDestinationA.data(), aDestinationA.size(), 0x55550000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aDestinationB.data(), aDestinationB.size(), 0x66660000U ^ static_cast<std::uint32_t>(aTypeIndex));
        PasswordExpander::ExpandPasswordByIndex(
            aType, aPassword.data(), nullptr, aDestinationA.data(), static_cast<unsigned int>(aPasswordLength));
        PasswordExpander::ExpandPasswordByIndex(
            aType, aPassword.data(), nullptr, aDestinationB.data(), static_cast<unsigned int>(aPasswordLength));

        if (!CheckEqual(aDestinationA.data(),
                        aDestinationB.data(),
                        aDestinationA.size(),
                        "PasswordExpander null worker",
                        aLoop,
                        aTypeIndex,
                        aPasswordLength)) {
          return 1;
        }

        FillPattern(aWorkerA.data(), aWorkerA.size(), 0x77770000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aWorkerB.data(), aWorkerB.size(), 0x88880000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aLongA.data(), aLongA.size(), 0x99990000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aLongB.data(), aLongB.size(), 0xAAAA0000U ^ static_cast<std::uint32_t>(aTypeIndex));
        PasswordExpander::ExpandPasswordBlocksByIndex(aType,
                                                      aPassword.data(),
                                                      static_cast<unsigned int>(aPasswordLength),
                                                      aWorkerA.data(),
                                                      aLongA.data(),
                                                      static_cast<unsigned int>(aLongA.size()));
        PasswordExpander::ExpandPasswordBlocksByIndex(aType,
                                                      aPassword.data(),
                                                      static_cast<unsigned int>(aPasswordLength),
                                                      aWorkerB.data(),
                                                      aLongB.data(),
                                                      static_cast<unsigned int>(aLongB.size()));

        if (!CheckEqual(aLongA.data(),
                        aLongB.data(),
                        aLongA.size(),
                        "PasswordExpander multi-block",
                        aLoop,
                        aTypeIndex,
                        aPasswordLength)) {
          return 1;
        }

        PasswordExpander::FillDoubledSource(
            aPassword.data(), static_cast<unsigned int>(aPasswordLength), aSourceA.data());
        std::memcpy(aSourceB.data(), aSourceA.data(), aSourceA.size());
        FillPattern(aWorkerA.data(), aWorkerA.size(), 0xBBBB0000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aWorkerB.data(), aWorkerB.size(), 0xCCCC0000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aDestinationA.data(), aDestinationA.size(), 0xDDDD0000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aDestinationB.data(), aDestinationB.size(), 0xEEEE0000U ^ static_cast<std::uint32_t>(aTypeIndex));
        ByteTwister::TwistBytesByIndex(
            aType, aSourceA.data(), aWorkerA.data(), aDestinationA.data(), PASSWORD_EXPANDED_SIZE);
        ByteTwister::TwistBytesByIndex(
            aType, aSourceB.data(), aWorkerB.data(), aDestinationB.data(), PASSWORD_EXPANDED_SIZE);

        if (!CheckEqual(aDestinationA.data(),
                        aDestinationB.data(),
                        aDestinationA.size(),
                        "ByteTwister direct",
                        aLoop,
                        aTypeIndex,
                        aPasswordLength)) {
          return 1;
        }

        PasswordExpander::FillDoubledSource(
            aPassword.data(), static_cast<unsigned int>(aPasswordLength), aAliasA.data());
        PasswordExpander::FillDoubledSource(
            aPassword.data(), static_cast<unsigned int>(aPasswordLength), aAliasB.data());
        FillPattern(aWorkerA.data(), aWorkerA.size(), 0x13570000U ^ static_cast<std::uint32_t>(aTypeIndex));
        FillPattern(aWorkerB.data(), aWorkerB.size(), 0x24680000U ^ static_cast<std::uint32_t>(aTypeIndex));
        ByteTwister::TwistBytesByIndex(
            aType, aAliasA.data(), aWorkerA.data(), aAliasA.data(), PASSWORD_EXPANDED_SIZE);
        ByteTwister::TwistBytesByIndex(
            aType, aAliasB.data(), aWorkerB.data(), aAliasB.data(), PASSWORD_EXPANDED_SIZE);

        if (!CheckEqual(aAliasA.data(),
                        aAliasB.data(),
                        aAliasA.size(),
                        "ByteTwister alias",
                        aLoop,
                        aTypeIndex,
                        aPasswordLength)) {
          return 1;
        }

        aDigest = (aDigest * 1099511628211ULL) ^ HashBytes(aAliasA.data(), aAliasA.size());
        aDigest = (aDigest * 1099511628211ULL) ^ HashBytes(aLongA.data(), aLongA.size());
      }
    }
  }

  std::cout << "[PASS] byte twister poisoned repeatability tests passed"
            << " loops=" << aLoops
            << " lengths=" << aPasswordLengths.size()
            << " types=" << ByteTwister::kTypeCount
            << " digest=" << aDigest << "\n";
  return 0;
}
