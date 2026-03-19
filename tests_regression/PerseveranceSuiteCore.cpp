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
#include "tests_regression/PerseveranceSuiteCore.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pSalt);
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

bool SameSummary(const peanutbutter::tests::games::GameRunSummary& pLeft,
                 const peanutbutter::tests::games::GameRunSummary& pRight) {
  return pLeft.mSuccessfulMoveCount == pRight.mSuccessfulMoveCount &&
         pLeft.mBrokenCount == pRight.mBrokenCount &&
         pLeft.mRuntimeStats.mStuck == pRight.mRuntimeStats.mStuck &&
         pLeft.mRuntimeStats.mTopple == pRight.mRuntimeStats.mTopple &&
         pLeft.mRuntimeStats.mUserMatch == pRight.mRuntimeStats.mUserMatch &&
         pLeft.mRuntimeStats.mCascadeMatch == pRight.mRuntimeStats.mCascadeMatch &&
         pLeft.mRuntimeStats.mGameStateOverflowCatastrophic == pRight.mRuntimeStats.mGameStateOverflowCatastrophic &&
         pLeft.mRuntimeStats.mCatastropheCaseA == pRight.mRuntimeStats.mCatastropheCaseA &&
         pLeft.mRuntimeStats.mCatastropheCaseB == pRight.mRuntimeStats.mCatastropheCaseB &&
         pLeft.mRuntimeStats.mCatastropheCaseC == pRight.mRuntimeStats.mCatastropheCaseC &&
         pLeft.mRuntimeStats.mGameStateOverflowCataclysmic == pRight.mRuntimeStats.mGameStateOverflowCataclysmic &&
         pLeft.mRuntimeStats.mGameStateOverflowApocalypse == pRight.mRuntimeStats.mGameStateOverflowApocalypse &&
         pLeft.mRuntimeStats.mPowerUpSpawned == pRight.mRuntimeStats.mPowerUpSpawned &&
         pLeft.mRuntimeStats.mPowerUpConsumed == pRight.mRuntimeStats.mPowerUpConsumed &&
         pLeft.mRuntimeStats.mPasswordExpanderWraps == pRight.mRuntimeStats.mPasswordExpanderWraps &&
         pLeft.mRuntimeStats.mDragonAttack == pRight.mRuntimeStats.mDragonAttack &&
         pLeft.mRuntimeStats.mPhoenixAttack == pRight.mRuntimeStats.mPhoenixAttack &&
         pLeft.mRuntimeStats.mWyvernAttack == pRight.mRuntimeStats.mWyvernAttack &&
         pLeft.mRuntimeStats.mBlueMoonCase == pRight.mRuntimeStats.mBlueMoonCase &&
         pLeft.mRuntimeStats.mHarvestMoon == pRight.mRuntimeStats.mHarvestMoon &&
         pLeft.mRuntimeStats.mImpossible == pRight.mRuntimeStats.mImpossible &&
         pLeft.mRuntimeStats.mPlantedMatchSolve == pRight.mRuntimeStats.mPlantedMatchSolve &&
         pLeft.mRuntimeStats.mInconsistentStateA == pRight.mRuntimeStats.mInconsistentStateA &&
         pLeft.mRuntimeStats.mInconsistentStateB == pRight.mRuntimeStats.mInconsistentStateB &&
         pLeft.mRuntimeStats.mInconsistentStateC == pRight.mRuntimeStats.mInconsistentStateC &&
         pLeft.mRuntimeStats.mInconsistentStateD == pRight.mRuntimeStats.mInconsistentStateD &&
         pLeft.mRuntimeStats.mInconsistentStateE == pRight.mRuntimeStats.mInconsistentStateE &&
         pLeft.mRuntimeStats.mInconsistentStateF == pRight.mRuntimeStats.mInconsistentStateF &&
         pLeft.mRuntimeStats.mInconsistentStateG == pRight.mRuntimeStats.mInconsistentStateG &&
         pLeft.mRuntimeStats.mInconsistentStateH == pRight.mRuntimeStats.mInconsistentStateH &&
         pLeft.mRuntimeStats.mInconsistentStateI == pRight.mRuntimeStats.mInconsistentStateI &&
         pLeft.mRuntimeStats.mInconsistentStateJ == pRight.mRuntimeStats.mInconsistentStateJ &&
         pLeft.mRuntimeStats.mInconsistentStateK == pRight.mRuntimeStats.mInconsistentStateK &&
         pLeft.mRuntimeStats.mInconsistentStateL == pRight.mRuntimeStats.mInconsistentStateL &&
         pLeft.mRuntimeStats.mInconsistentStateM == pRight.mRuntimeStats.mInconsistentStateM &&
         pLeft.mRuntimeStats.mInconsistentStateN == pRight.mRuntimeStats.mInconsistentStateN &&
         pLeft.mRuntimeStats.mInconsistentStateO == pRight.mRuntimeStats.mInconsistentStateO;
}

bool RunPerseveranceCase(const peanutbutter::tests::games::GameCatalogEntry& pEntry,
                        int pLoops,
                        int pDataLength,
                        std::uint64_t* pDigest,
                        peanutbutter::games::GameBoard::RuntimeStats* pRuntimeTotals) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pEntry.mSalt);

    peanutbutter::tests::games::GameRunSummary aSummaryA;
    peanutbutter::tests::games::GameRunSummary aSummaryB;
    if (!peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
            pEntry.mGameIndex, aInput.data(), pDataLength, aOutputA.data(), &aSummaryA)) {
      std::cerr << "[FAIL] invalid game byte length=" << pDataLength << "\n";
      return false;
    }
    if (!peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
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
      std::cerr << "[FAIL] " << aName << " perseverance output mismatch at loop " << aLoop
                << " bytes=" << pDataLength << "\n";
      return false;
    }
    if (!SameSummary(aSummaryA, aSummaryB)) {
      std::cerr << "[FAIL] " << aName << " perseverance stats mismatch at loop " << aLoop
                << " bytes=" << pDataLength << "\n";
      return false;
    }

    if (pRuntimeTotals != nullptr) {
      peanutbutter::tests::games::AccumulateRuntimeStats(aSummaryA.mRuntimeStats, pRuntimeTotals);
    }
    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
  }

  std::cout << "[PASS] " << peanutbutter::tests::games::GetGameName(pEntry.mGameIndex)
            << " perseverance loops=" << pLoops << " bytes=" << pDataLength << "\n";
  return true;
}

bool RunPerseveranceGames(std::string_view pMode, int pLoops, int pDataLength) {
  if (pMode != "games" && pMode != "all") {
    return false;
  }
  if (!peanutbutter::tests::games::IsValidGameByteLength(pDataLength)) {
    std::cerr << "[FAIL] game byte length must be a positive multiple of "
              << peanutbutter::games::GameBoard::kSeedBufferCapacity
              << " bytes, got " << pDataLength << "\n";
    return false;
  }

  std::uint64_t aDigest = 1469598103934665603ULL;
  peanutbutter::games::GameBoard::RuntimeStats aRuntimeTotals{};

  for (const peanutbutter::tests::games::GameCatalogEntry& aEntry : peanutbutter::tests::games::kAllGames) {
    if (!RunPerseveranceCase(aEntry, pLoops, pDataLength, &aDigest, &aRuntimeTotals)) {
      return false;
    }
  }

  std::cout << "[PASS] game perseverance tests passed"
            << " loops=" << pLoops << " bytes=" << pDataLength
            << " games=" << peanutbutter::tests::games::kAllGames.size()
            << " digest=" << aDigest << "\n";
  std::cout << "  stuck=" << aRuntimeTotals.mStuck
            << " topple=" << aRuntimeTotals.mTopple
            << " user_match=" << aRuntimeTotals.mUserMatch
            << " cascade_match=" << aRuntimeTotals.mCascadeMatch
            << " overflow_catastrophic=" << aRuntimeTotals.mGameStateOverflowCatastrophic
            << " catastrophe_case_a=" << aRuntimeTotals.mCatastropheCaseA
            << " catastrophe_case_b=" << aRuntimeTotals.mCatastropheCaseB
            << " catastrophe_case_c=" << aRuntimeTotals.mCatastropheCaseC
            << " overflow_cataclysmic=" << aRuntimeTotals.mGameStateOverflowCataclysmic
            << " overflow_apocalypse=" << aRuntimeTotals.mGameStateOverflowApocalypse
            << " powerup_spawned=" << aRuntimeTotals.mPowerUpSpawned
            << " powerup_consumed=" << aRuntimeTotals.mPowerUpConsumed
            << " wraps=" << aRuntimeTotals.mPasswordExpanderWraps
            << " dragon=" << aRuntimeTotals.mDragonAttack
            << " phoenix=" << aRuntimeTotals.mPhoenixAttack
            << " wyvern=" << aRuntimeTotals.mWyvernAttack
            << " blue_moon_case=" << aRuntimeTotals.mBlueMoonCase
            << " harvest_moon=" << aRuntimeTotals.mHarvestMoon
            << " impossible=" << aRuntimeTotals.mImpossible
            << " planted_match_solve=" << aRuntimeTotals.mPlantedMatchSolve << "\n";
  std::cout << "  inconsistent_a=" << aRuntimeTotals.mInconsistentStateA
            << " inconsistent_b=" << aRuntimeTotals.mInconsistentStateB
            << " inconsistent_c=" << aRuntimeTotals.mInconsistentStateC
            << " inconsistent_d=" << aRuntimeTotals.mInconsistentStateD
            << " inconsistent_e=" << aRuntimeTotals.mInconsistentStateE
            << " inconsistent_f=" << aRuntimeTotals.mInconsistentStateF
            << " inconsistent_g=" << aRuntimeTotals.mInconsistentStateG
            << " inconsistent_h=" << aRuntimeTotals.mInconsistentStateH
            << " inconsistent_i=" << aRuntimeTotals.mInconsistentStateI
            << " inconsistent_j=" << aRuntimeTotals.mInconsistentStateJ
            << " inconsistent_k=" << aRuntimeTotals.mInconsistentStateK
            << " inconsistent_l=" << aRuntimeTotals.mInconsistentStateL
            << " inconsistent_m=" << aRuntimeTotals.mInconsistentStateM
            << " inconsistent_n=" << aRuntimeTotals.mInconsistentStateN
            << " inconsistent_o=" << aRuntimeTotals.mInconsistentStateO << "\n";
  std::cout << "  inconsistent_flags=0x" << std::hex << std::uppercase
            << peanutbutter::tests::games::InconsistentFlags(aRuntimeTotals)
            << std::dec << std::nouppercase << "\n";
  std::cout << "  game_flags=0x" << std::hex << std::uppercase
            << peanutbutter::tests::games::RuntimeFlags(aRuntimeTotals) << std::dec << std::nouppercase << "\n";
  return true;
}

}  // namespace

namespace peanutbutter::tests::perseverance {

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

}  // namespace peanutbutter::tests::perseverance
