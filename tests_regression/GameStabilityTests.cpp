#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "tests/common/GameCatalog.hpp"
#include "tests/common/GameChunkRunner.hpp"
#include "tests/common/Tests.hpp"

namespace {

struct Histogram256 {
  std::array<std::uint64_t, 256> mCounts{};
};

struct StabilityAggregate {
  std::uint64_t mRuns = 0U;
  std::uint64_t mRepeatabilityChecks = 0U;
  std::uint64_t mHistogramChecks = 0U;
  std::uint64_t mRepeatabilityFailures = 0U;
  std::uint64_t mRuntimeRepeatabilityFailures = 0U;
  std::uint64_t mHistogramDriftCount = 0U;
  std::uint64_t mSuccessfulMoveCount = 0U;
  std::uint64_t mBrokenCount = 0U;
  peanutbutter::games::GameBoard::RuntimeStats mRuntimeStats{};
  std::uint64_t mOutputDigest = 1469598103934665603ULL;
  std::uint64_t mRuntimeDigest = 1469598103934665603ULL;
};

std::uint64_t HashMix(std::uint64_t pDigest, std::uint64_t pValue) {
  return (pDigest * 1099511628211ULL) ^ pValue;
}

std::uint32_t SeedState(int pRunIndex, int pSalt) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pRunIndex + pSalt);
  return 0xA24BAED4U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
}

void FillInput(std::vector<unsigned char>* pBytes, int pRunIndex, int pSalt) {
  std::uint32_t aState = SeedState(pRunIndex + static_cast<int>(pBytes->size()), pSalt);
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

Histogram256 BuildHistogram(const std::vector<unsigned char>& pBytes) {
  Histogram256 aHistogram;
  for (unsigned char aByte : pBytes) {
    ++aHistogram.mCounts[aByte];
  }
  return aHistogram;
}

bool HistogramsMatch(const Histogram256& pA, const Histogram256& pB) {
  return pA.mCounts == pB.mCounts;
}

std::uint64_t HashBytes(const std::vector<unsigned char>& pBytes) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (unsigned char aByte : pBytes) {
    aDigest = HashMix(aDigest, static_cast<std::uint64_t>(aByte));
  }
  return aDigest;
}

std::uint64_t HashRuntime(const peanutbutter::tests::games::GameRunSummary& pSummary) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  aDigest = HashMix(aDigest, static_cast<std::uint64_t>(pSummary.mSuccessfulMoveCount));
  aDigest = HashMix(aDigest, static_cast<std::uint64_t>(pSummary.mBrokenCount));
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mStuck);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mTopple);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mUserMatch);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mCascadeMatch);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mGameStateOverflowCatastrophic);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mCatastropheCaseA);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mCatastropheCaseB);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mCatastropheCaseC);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mGameStateOverflowCataclysmic);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mGameStateOverflowApocalypse);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mPowerUpSpawned);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mPowerUpConsumed);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mPasswordExpanderWraps);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mDragonAttack);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mPhoenixAttack);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mWyvernAttack);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mBlueMoonCase);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mHarvestMoon);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mImpossible);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mPlantedMatchSolve);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateA);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateB);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateC);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateD);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateE);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateF);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateG);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateH);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateI);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateJ);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateK);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateL);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateM);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateN);
  aDigest = HashMix(aDigest, pSummary.mRuntimeStats.mInconsistentStateO);
  return aDigest;
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

void Accumulate(StabilityAggregate* pAggregate,
                const peanutbutter::tests::games::GameRunSummary& pSummary,
                const std::vector<unsigned char>& pOutput) {
  pAggregate->mRuns += 1U;
  pAggregate->mRepeatabilityChecks += 1U;
  pAggregate->mHistogramChecks += 1U;
  pAggregate->mSuccessfulMoveCount += static_cast<std::uint64_t>(pSummary.mSuccessfulMoveCount);
  pAggregate->mBrokenCount += static_cast<std::uint64_t>(pSummary.mBrokenCount);
  peanutbutter::tests::games::AccumulateRuntimeStats(pSummary.mRuntimeStats, &pAggregate->mRuntimeStats);
  pAggregate->mOutputDigest = HashMix(pAggregate->mOutputDigest, HashBytes(pOutput));
  pAggregate->mRuntimeDigest = HashMix(pAggregate->mRuntimeDigest, HashRuntime(pSummary));
}

}  // namespace

int main(int argc, char** argv) {
  int aRuns = 100;
  bool aHasHardFailure = false;
  if (argc >= 2) {
    aRuns = std::atoi(argv[1]);
  }
  if (aRuns <= 0) {
    std::cerr << "[FAIL] run count must be positive\n";
    return 1;
  }

  const int aDataLength =
      (peanutbutter::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? peanutbutter::games::GameBoard::kSeedBufferCapacity
                                                                : peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;
  if (!peanutbutter::tests::games::IsValidGameByteLength(aDataLength)) {
    std::cerr << "[FAIL] game byte length must be a positive multiple of "
              << peanutbutter::games::GameBoard::kSeedBufferCapacity
              << " bytes, got " << aDataLength << "\n";
    return 1;
  }

  std::cout << "# Game Stability Tests\n";
  std::cout << "# runs_per_game=" << aRuns << "\n";
  std::cout << "# bytes_per_game=" << aDataLength << "\n";

  for (std::size_t aGameCatalogIndex = 0; aGameCatalogIndex < peanutbutter::tests::games::kAllGames.size();
       ++aGameCatalogIndex) {
    const peanutbutter::tests::games::GameCatalogEntry& aEntry = peanutbutter::tests::games::kAllGames[aGameCatalogIndex];
    const char* aName = peanutbutter::tests::games::GetGameName(aEntry.mGameIndex);
    StabilityAggregate aAggregate;

    std::cout << "[GAME] " << aGameCatalogIndex << " " << aName << "\n";

    for (int aRun = 0; aRun < aRuns; ++aRun) {
      std::vector<unsigned char> aInput(static_cast<std::size_t>(aDataLength), 0U);
      FillInput(&aInput, aRun, aEntry.mSalt);

      std::vector<unsigned char> aExpandedSeedA(static_cast<std::size_t>(aDataLength), 0U);
      std::vector<unsigned char> aExpandedSeedB(static_cast<std::size_t>(aDataLength), 0U);
      std::vector<unsigned char> aOutputA(static_cast<std::size_t>(aDataLength), 0U);
      std::vector<unsigned char> aOutputB(static_cast<std::size_t>(aDataLength), 0U);
      peanutbutter::tests::games::GameRunSummary aSummaryA;
      peanutbutter::tests::games::GameRunSummary aSummaryB;

      if (!peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
              aEntry.mGameIndex, aInput.data(), aDataLength, aOutputA.data(), &aSummaryA, aExpandedSeedA.data()) ||
          !peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
              aEntry.mGameIndex, aInput.data(), aDataLength, aOutputB.data(), &aSummaryB, aExpandedSeedB.data())) {
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " invalid byte length at run " << aRun << "\n";
        continue;
      }

      if (aOutputA != aOutputB) {
        ++aAggregate.mRepeatabilityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " output repeatability mismatch at run " << aRun << "\n";
        continue;
      }
      if (!SameSummary(aSummaryA, aSummaryB)) {
        ++aAggregate.mRuntimeRepeatabilityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " runtime repeatability mismatch at run " << aRun << "\n";
        continue;
      }

      const Histogram256 aSeedHistogramA = BuildHistogram(aExpandedSeedA);
      const Histogram256 aSeedHistogramB = BuildHistogram(aExpandedSeedB);
      const Histogram256 aOutputHistogramA = BuildHistogram(aOutputA);
      const Histogram256 aOutputHistogramB = BuildHistogram(aOutputB);
      if (!HistogramsMatch(aSeedHistogramA, aOutputHistogramA) ||
          !HistogramsMatch(aSeedHistogramB, aOutputHistogramB)) {
        ++aAggregate.mHistogramDriftCount;
        std::cerr << "[WARN] " << aName << " histogram drift at run " << aRun << "\n";
      }

      Accumulate(&aAggregate, aSummaryA, aOutputA);
    }

    std::cout << "  runs=" << aAggregate.mRuns
              << " repeatability_checks=" << aAggregate.mRepeatabilityChecks
              << " histogram_checks=" << aAggregate.mHistogramChecks << "\n";
    std::cout << "  repeatability_failures=" << aAggregate.mRepeatabilityFailures
              << " runtime_repeatability_failures=" << aAggregate.mRuntimeRepeatabilityFailures
              << " histogram_drift=" << aAggregate.mHistogramDriftCount << "\n";
    std::cout << "  successful_moves=" << aAggregate.mSuccessfulMoveCount
              << " broken=" << aAggregate.mBrokenCount << "\n";
    std::cout << "  stuck=" << aAggregate.mRuntimeStats.mStuck
              << " topple=" << aAggregate.mRuntimeStats.mTopple
              << " user_match=" << aAggregate.mRuntimeStats.mUserMatch
              << " cascade_match=" << aAggregate.mRuntimeStats.mCascadeMatch << "\n";
    std::cout << "  overflow_catastrophic=" << aAggregate.mRuntimeStats.mGameStateOverflowCatastrophic
              << " catastrophe_case_a=" << aAggregate.mRuntimeStats.mCatastropheCaseA
              << " catastrophe_case_b=" << aAggregate.mRuntimeStats.mCatastropheCaseB
              << " catastrophe_case_c=" << aAggregate.mRuntimeStats.mCatastropheCaseC
              << " overflow_cataclysmic=" << aAggregate.mRuntimeStats.mGameStateOverflowCataclysmic
              << " overflow_apocalypse=" << aAggregate.mRuntimeStats.mGameStateOverflowApocalypse << "\n";
    std::cout << "  powerup_spawned=" << aAggregate.mRuntimeStats.mPowerUpSpawned
              << " powerup_consumed=" << aAggregate.mRuntimeStats.mPowerUpConsumed
              << " wraps=" << aAggregate.mRuntimeStats.mPasswordExpanderWraps
              << " dragon=" << aAggregate.mRuntimeStats.mDragonAttack
              << " phoenix=" << aAggregate.mRuntimeStats.mPhoenixAttack
              << " wyvern=" << aAggregate.mRuntimeStats.mWyvernAttack
              << " blue_moon_case=" << aAggregate.mRuntimeStats.mBlueMoonCase
              << " harvest_moon=" << aAggregate.mRuntimeStats.mHarvestMoon
              << " impossible=" << aAggregate.mRuntimeStats.mImpossible
              << " planted_match_solve=" << aAggregate.mRuntimeStats.mPlantedMatchSolve << "\n";
    std::cout << "  inconsistent_a=" << aAggregate.mRuntimeStats.mInconsistentStateA
              << " inconsistent_b=" << aAggregate.mRuntimeStats.mInconsistentStateB
              << " inconsistent_c=" << aAggregate.mRuntimeStats.mInconsistentStateC
              << " inconsistent_d=" << aAggregate.mRuntimeStats.mInconsistentStateD
              << " inconsistent_e=" << aAggregate.mRuntimeStats.mInconsistentStateE
              << " inconsistent_f=" << aAggregate.mRuntimeStats.mInconsistentStateF
              << " inconsistent_g=" << aAggregate.mRuntimeStats.mInconsistentStateG
              << " inconsistent_h=" << aAggregate.mRuntimeStats.mInconsistentStateH
              << " inconsistent_i=" << aAggregate.mRuntimeStats.mInconsistentStateI
              << " inconsistent_j=" << aAggregate.mRuntimeStats.mInconsistentStateJ
              << " inconsistent_k=" << aAggregate.mRuntimeStats.mInconsistentStateK
              << " inconsistent_l=" << aAggregate.mRuntimeStats.mInconsistentStateL
              << " inconsistent_m=" << aAggregate.mRuntimeStats.mInconsistentStateM
              << " inconsistent_n=" << aAggregate.mRuntimeStats.mInconsistentStateN
              << " inconsistent_o=" << aAggregate.mRuntimeStats.mInconsistentStateO << "\n";
    std::cout << "  inconsistent_flags=0x" << std::hex << std::uppercase
              << peanutbutter::tests::games::InconsistentFlags(aAggregate.mRuntimeStats)
              << std::dec << std::nouppercase << "\n";
    std::cout << "  game_flags=0x" << std::hex << std::uppercase
              << peanutbutter::tests::games::RuntimeFlags(aAggregate.mRuntimeStats)
              << std::dec << std::nouppercase << "\n";
    std::cout << "  output_digest=" << aAggregate.mOutputDigest
              << " runtime_digest=" << aAggregate.mRuntimeDigest << "\n";
  }

  if (aHasHardFailure) {
    std::cerr << "[FAIL] game stability tests found hard stability failures\n";
    return 1;
  }

  std::cout << "[PASS] game stability tests passed\n";
  return 0;
}
