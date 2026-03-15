#include <cstddef>
#include <cstdint>
#include <array>
#include <iostream>
#include <string_view>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/games/minigames/MatchThreeSlideIslands.hpp"
#include "src/games/minigames/MatchThreeSlideStreaks.hpp"
#include "src/games/minigames/MatchThreeSwapIslands.hpp"
#include "src/games/minigames/MatchThreeSwapStreaks.hpp"
#include "src/games/minigames/MatchThreeTapIslands.hpp"
#include "src/games/minigames/MatchThreeTapStreaks.hpp"
#include "tests/common/Tests.hpp"
#include "tests/perseverance/PerseveranceSuiteCore.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0x7F4A7C15U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
}

void FillInput(std::vector<unsigned char>* pBytes, int pTrial, int pSalt) {
  std::uint32_t aState = SeedState(pTrial + pSalt + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

bool SameByteMultiset(const std::vector<unsigned char>& pInput,
                     const std::vector<unsigned char>& pOutput,
                     int* pMismatchByte,
                     int* pInputCount,
                     int* pOutputCount) {
  if (pInput.size() != pOutput.size()) {
    if (pMismatchByte != nullptr) {
      *pMismatchByte = -1;
    }
    return false;
  }

  std::array<int, 256> aInputCounts{};
  std::array<int, 256> aOutputCounts{};
  for (unsigned char aByte : pInput) {
    ++aInputCounts[aByte];
  }
  for (unsigned char aByte : pOutput) {
    ++aOutputCounts[aByte];
  }

  for (int aByte = 0; aByte < 256; ++aByte) {
    if (aInputCounts[aByte] != aOutputCounts[aByte]) {
      if (pMismatchByte != nullptr) {
        *pMismatchByte = aByte;
      }
      if (pInputCount != nullptr) {
        *pInputCount = aInputCounts[aByte];
      }
      if (pOutputCount != nullptr) {
        *pOutputCount = aOutputCounts[aByte];
      }
      return false;
    }
  }
  return true;
}

template <typename TGame>
bool RunPerseveranceCase(const char* pName, int pLoops, int pDataLength, int pSalt, std::uint64_t* pDigest) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aSeed(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutput(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pSalt);

    MersenneCounter aCounter;
    TGame aGame(&aCounter);
    aGame.Seed(aInput.data(), pDataLength);
    aGame.Get(aOutput.data(), pDataLength);

    MersenneCounter aSeedCounter;
    aSeedCounter.Seed(aInput.data(), pDataLength);
    aSeedCounter.Get(aSeed.data(), pDataLength);

    int aMismatchByte = -1;
    int aInputCount = 0;
    int aOutputCount = 0;
    if (!SameByteMultiset(aSeed, aOutput, &aMismatchByte, &aInputCount, &aOutputCount)) {
      std::cerr << "[FAIL] " << pName << " perseverance mismatch at loop " << aLoop
                << " bytes=" << pDataLength;
      if (aMismatchByte >= 0) {
        std::cerr << " mismatch_byte=" << aMismatchByte
                  << " input_count=" << aInputCount
                  << " output_count=" << aOutputCount;
      }
      std::cerr << "\n";
      return false;
    }

    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutput) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
  }

  std::cout << "[PASS] " << pName << " perseverance loops=" << pLoops << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunPerseveranceGames(std::string_view pMode, int pLoops, int pDataLength) {
  if (pMode != "games" && pMode != "all") {
    return false;
  }

  std::uint64_t aDigest = 1469598103934665603ULL;

  if (!RunPerseveranceCase<bread::games::MatchThreeTapStreaks>(
          "MatchThreeTapStreaks", pLoops, pDataLength, 11, &aDigest) ||
      !RunPerseveranceCase<bread::games::MatchThreeTapIslands>(
          "MatchThreeTapIslands", pLoops, pDataLength, 23, &aDigest) ||
      !RunPerseveranceCase<bread::games::MatchThreeSwapStreaks>(
          "MatchThreeSwapStreaks", pLoops, pDataLength, 37, &aDigest) ||
      !RunPerseveranceCase<bread::games::MatchThreeSwapIslands>(
          "MatchThreeSwapIslands", pLoops, pDataLength, 41, &aDigest) ||
      !RunPerseveranceCase<bread::games::MatchThreeSlideStreaks>(
          "MatchThreeSlideStreaks", pLoops, pDataLength, 53, &aDigest) ||
      !RunPerseveranceCase<bread::games::MatchThreeSlideIslands>(
          "MatchThreeSlideIslands", pLoops, pDataLength, 67, &aDigest)) {
    return false;
  }

  std::cout << "[PASS] game perseverance tests passed"
            << " loops=" << pLoops << " bytes=" << pDataLength << " digest=" << aDigest << "\n";
  return true;
}

}  // namespace

namespace bread::tests::perseverance {

bool RunDigestPerseveranceSuite(std::string_view pMode, int pLoops, int pDataLength) {
  if (pMode.empty()) {
    pMode = "games";
  }

  if (!RunPerseveranceGames(pMode, pLoops, pDataLength)) {
    if (pMode == "games") {
      std::cerr << "[FAIL] game perseverance mismatch\n";
    } else if (pMode == "all") {
      std::cerr << "[FAIL] perseverance mismatch\n";
    } else {
      std::cerr << "[FAIL] unknown perseverance mode=" << pMode << " (expected games or all)\n";
    }
    return false;
  }

  return true;
}

}  // namespace bread::tests::perseverance
