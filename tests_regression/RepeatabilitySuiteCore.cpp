#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "tests/common/CounterSeedBuffer.hpp"
#include "tests/common/GameCatalog.hpp"
#include "tests/common/GameChunkRunner.hpp"
#include "tests/common/Tests.hpp"
#include "tests_regression/DigestRepeatabilityJobs.hpp"
#include "tests_regression/RepeatabilitySuiteCore.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pSalt);
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

bool RunGameRepeatabilityCase(const peanutbutter::tests::games::GameCatalogEntry& pEntry,
                             int pLoops,
                             int pDataLength,
                             int* pMoveTotal,
                             int* pBrokenTotal,
                             std::uint64_t* pDigest) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pEntry.mSalt);

    peanutbutter::tests::games::GameRunSummary aSummaryA;
    peanutbutter::tests::games::GameRunSummary aSummaryB;
    if (!peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
            pEntry.mGameIndex, aInput.data(), pDataLength, aOutputA.data(), &aSummaryA) ||
        !peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
            pEntry.mGameIndex, aInput.data(), pDataLength, aOutputB.data(), &aSummaryB)) {
      std::cerr << "[FAIL] invalid game byte length=" << pDataLength << "\n";
      return false;
    }

    const char* aName = peanutbutter::tests::games::GetGameName(pEntry.mGameIndex);
    if (aName == nullptr || aName[0] == '\0') {
      std::cerr << "[FAIL] game name missing at loop " << aLoop << "\n";
      return false;
    }

    if (aOutputA != aOutputB) {
      std::cerr << "[FAIL] " << aName << " output mismatch at loop " << aLoop << "\n";
      return false;
    }
    if (aSummaryA.mSuccessfulMoveCount != aSummaryB.mSuccessfulMoveCount) {
      std::cerr << "[FAIL] " << aName << " move-count mismatch at loop " << aLoop << "\n";
      return false;
    }
    if (aSummaryA.mBrokenCount != aSummaryB.mBrokenCount) {
      std::cerr << "[FAIL] " << aName << " broken-count mismatch at loop " << aLoop << "\n";
      return false;
    }

    if (pMoveTotal != nullptr) {
      *pMoveTotal += aSummaryA.mSuccessfulMoveCount;
    }
    if (pBrokenTotal != nullptr) {
      *pBrokenTotal += aSummaryA.mBrokenCount;
    }
    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
      *pDigest ^= static_cast<std::uint64_t>(aSummaryA.mSuccessfulMoveCount & 0xFFFF);
      *pDigest ^= static_cast<std::uint64_t>((aSummaryA.mBrokenCount & 0xFFFF) << 16U);
    }
  }

  std::cout << "[PASS] " << peanutbutter::tests::games::GetGameName(pEntry.mGameIndex)
            << " repeatable loops=" << pLoops << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunDigestRepeatabilityMode(int pLoops, int pDataLength) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    if (!peanutbutter::tests::repeatability::RunDigestRepeatabilityShort() ||
        !peanutbutter::tests::repeatability::RunDigestRepeatabilityMedium() ||
        !peanutbutter::tests::repeatability::RunDigestRepeatabilityOversized() ||
        !peanutbutter::tests::repeatability::RunDigestRepeatabilityContaminated() ||
        !peanutbutter::tests::repeatability::RunDigestRepeatabilityManual()) {
      return false;
    }
  }

  std::cout << "[PASS] digest repeatability suite passed loops=" << pLoops
            << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunGameRepeatabilityMode(int pLoops, int pDataLength, bool pRunFull) {
  (void)pRunFull;
  if (!peanutbutter::tests::games::IsValidGameByteLength(pDataLength)) {
    std::cerr << "[FAIL] game byte length must be a positive multiple of "
              << peanutbutter::games::GameBoard::kSeedBufferCapacity
              << " bytes, got " << pDataLength << "\n";
    return false;
  }
  int aMoveTotal = 0;
  int aBrokenTotal = 0;
  std::uint64_t aDigest = 1469598103934665603ULL;

  bool aOk = true;
  for (const peanutbutter::tests::games::GameCatalogEntry& aEntry : peanutbutter::tests::games::kAllGames) {
    if (!RunGameRepeatabilityCase(aEntry, pLoops, pDataLength, &aMoveTotal, &aBrokenTotal, &aDigest)) {
      aOk = false;
      break;
    }
  }

  if (!aOk) {
    return false;
  }

  std::cout << "[PASS] game repeatability tests passed"
            << " loops=" << pLoops << " bytes=" << pDataLength << " games="
            << peanutbutter::tests::games::kAllGames.size()
            << " moves=" << aMoveTotal << " broken=" << aBrokenTotal << " digest=" << aDigest << "\n";
  return true;
}

}  // namespace

namespace peanutbutter::tests::repeatability {

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

}  // namespace peanutbutter::tests::repeatability
