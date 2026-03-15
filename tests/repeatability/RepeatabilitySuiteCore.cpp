#include <cstddef>
#include <cstdint>
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
#include "tests/repeatability/DigestRepeatabilityJobs.hpp"
#include "tests/repeatability/RepeatabilitySuiteCore.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0xD1B54A32U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
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

template <typename TGame>
bool RunGameRepeatabilityCase(const char* pName,
                             int pLoops,
                             int pDataLength,
                             int pSalt,
                             int* pMoveTotal,
                             int* pBrokenTotal,
                             std::uint64_t* pDigest) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pSalt);

    MersenneCounter aCounterA;
    TGame aGameA(&aCounterA);
    aGameA.Seed(aInput.data(), pDataLength);
    aGameA.Get(aOutputA.data(), pDataLength);

    MersenneCounter aCounterB;
    TGame aGameB(&aCounterB);
    aGameB.Seed(aInput.data(), pDataLength);
    aGameB.Get(aOutputB.data(), pDataLength);

    if (aOutputA != aOutputB) {
      std::cerr << "[FAIL] " << pName << " output mismatch at loop " << aLoop << "\n";
      return false;
    }
    if (aGameA.SuccessfulMoveCount() != aGameB.SuccessfulMoveCount()) {
      std::cerr << "[FAIL] " << pName << " move-count mismatch at loop " << aLoop << "\n";
      return false;
    }
    if (aGameA.BrokenCount() != aGameB.BrokenCount()) {
      std::cerr << "[FAIL] " << pName << " broken-count mismatch at loop " << aLoop << "\n";
      return false;
    }

    if (pMoveTotal != nullptr) {
      *pMoveTotal += aGameA.SuccessfulMoveCount();
    }
    if (pBrokenTotal != nullptr) {
      *pBrokenTotal += aGameA.BrokenCount();
    }
    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
      *pDigest ^= static_cast<std::uint64_t>(aGameA.SuccessfulMoveCount() & 0xFFFF);
      *pDigest ^= static_cast<std::uint64_t>((aGameA.BrokenCount() & 0xFFFF) << 16U);
    }
  }

  std::cout << "[PASS] " << pName << " repeatable loops=" << pLoops << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunDigestRepeatabilityMode(int pLoops, int pDataLength) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    if (!bread::tests::repeatability::RunDigestRepeatabilityShort() ||
        !bread::tests::repeatability::RunDigestRepeatabilityMedium() ||
        !bread::tests::repeatability::RunDigestRepeatabilityOversized() ||
        !bread::tests::repeatability::RunDigestRepeatabilityContaminated() ||
        !bread::tests::repeatability::RunDigestRepeatabilityManual()) {
      return false;
    }
  }

  std::cout << "[PASS] digest repeatability suite passed loops=" << pLoops
            << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunGameRepeatabilityMode(int pLoops, int pDataLength, bool pRunFull) {
  int aMoveTotal = 0;
  int aBrokenTotal = 0;
  std::uint64_t aDigest = 1469598103934665603ULL;

  bool aOk = RunGameRepeatabilityCase<bread::games::MatchThreeTapStreaks>(
                "MatchThreeTapStreaks", pLoops, pDataLength, 11, &aMoveTotal, &aBrokenTotal, &aDigest) &&
            RunGameRepeatabilityCase<bread::games::MatchThreeTapIslands>(
                "MatchThreeTapIslands", pLoops, pDataLength, 23, &aMoveTotal, &aBrokenTotal, &aDigest);

  if (aOk && pRunFull) {
    aOk = RunGameRepeatabilityCase<bread::games::MatchThreeSwapStreaks>(
            "MatchThreeSwapStreaks", pLoops, pDataLength, 37, &aMoveTotal, &aBrokenTotal, &aDigest) &&
          RunGameRepeatabilityCase<bread::games::MatchThreeSwapIslands>(
              "MatchThreeSwapIslands", pLoops, pDataLength, 41, &aMoveTotal, &aBrokenTotal, &aDigest) &&
          RunGameRepeatabilityCase<bread::games::MatchThreeSlideStreaks>(
              "MatchThreeSlideStreaks", pLoops, pDataLength, 53, &aMoveTotal, &aBrokenTotal, &aDigest) &&
          RunGameRepeatabilityCase<bread::games::MatchThreeSlideIslands>(
              "MatchThreeSlideIslands", pLoops, pDataLength, 67, &aMoveTotal, &aBrokenTotal, &aDigest);
  } else if (!pRunFull) {
    std::cout << "[SKIP] heavy game variants disabled (set GAME_REPEATABILITY_FULL_ENABLED=true)" << "\n";
  }

  if (!aOk) {
    return false;
  }

  std::cout << "[PASS] game repeatability tests passed"
            << " loops=" << pLoops << " bytes=" << pDataLength << " full=" << (pRunFull ? 1 : 0)
            << " moves=" << aMoveTotal << " broken=" << aBrokenTotal << " digest=" << aDigest << "\n";
  return true;
}

}  // namespace

namespace bread::tests::repeatability {

bool RunDigestRepeatabilitySuite(std::string_view pMode, int pLoops, int pDataLength, bool pRunFullGameVariants) {
  if (pMode.empty()) {
    pMode = "digest";
  }

  const bool aRunDigest = (pMode == "digest" || pMode == "all");
  const bool aRunGames = (pMode == "games" || pMode == "all");
  if (!aRunDigest && !aRunGames) {
    std::cerr << "[FAIL] unknown repeatability mode=" << pMode << " (expected digest, games, or all)\n";
    return false;
  }

  if (aRunDigest && !RunDigestRepeatabilityMode(pLoops, pDataLength)) {
    return false;
  }

  if (aRunGames && !RunGameRepeatabilityMode(pLoops, pDataLength, pRunFullGameVariants)) {
    return false;
  }

  return true;
}

}  // namespace bread::tests::repeatability
