#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "src/Tables/counters/MersenneCounter.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/maze/MazePolicy.hpp"
#include "tests/common/CounterSeedBuffer.hpp"

namespace {

struct Histogram256 {
  std::array<std::uint64_t, 256> mCounts{};
};

struct StabilityAggregate {
  std::uint64_t mRuns = 0U;
  std::uint64_t mRepeatabilityChecks = 0U;
  std::uint64_t mHistogramChecks = 0U;
  std::uint64_t mConnectivityChecks = 0U;
  std::uint64_t mRepeatabilityFailures = 0U;
  std::uint64_t mRuntimeRepeatabilityFailures = 0U;
  std::uint64_t mHistogramDriftCount = 0U;
  std::uint64_t mConnectivityFailures = 0U;
  std::uint64_t mTopologyFailures = 0U;
  std::uint64_t mVictories = 0U;
  std::uint64_t mDeaths = 0U;
  std::uint64_t mFlush = 0U;
  std::uint64_t mEmptyFlush = 0U;
  std::uint64_t mTilesFlushedRegular = 0U;
  std::uint64_t mTilesFlushedStalled = 0U;
  std::uint64_t mTilesPaintedValidScenario = 0U;
  std::uint64_t mInconsistentStateA = 0U;
  std::uint64_t mInconsistentStateB = 0U;
  std::uint64_t mInconsistentStateC = 0U;
  std::uint64_t mInconsistentStateD = 0U;
  std::uint64_t mInconsistentStateE = 0U;
  std::uint64_t mSimulationStallCataclysmic = 0U;
  std::uint64_t mSimulationStallApocalypse = 0U;
  std::uint64_t mStarBurst = 0U;
  std::uint64_t mChaosStorm = 0U;
  std::uint64_t mCometTrailsHorizontal = 0U;
  std::uint64_t mCometTrailsVertical = 0U;
  std::uint64_t mSimulationCount = 0U;
  std::uint64_t mRobotLifeCount = 0U;
  std::uint64_t mTotalRobotWalk = 0U;
  std::uint64_t mShortestRobotWalk = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t mLongestRobotWalk = 0U;
  std::uint64_t mSharkKillCount = 0U;
  std::uint64_t mTotalSharkMovesBeforeKill = 0U;
  std::uint64_t mOutputDigest = 1469598103934665603ULL;
  std::uint64_t mRuntimeDigest = 1469598103934665603ULL;
  std::uint64_t mTopologyDigest = 1469598103934665603ULL;
};

std::uint64_t HashMix(std::uint64_t pDigest, std::uint64_t pValue) {
  return (pDigest * 1099511628211ULL) ^ pValue;
}

std::uint32_t SeedState(int pRunIndex, int pGameIndex) {
  return 0xC13FA9A9U ^ (static_cast<std::uint32_t>(pRunIndex) * 0x9E3779B9U) ^
         (static_cast<std::uint32_t>(pGameIndex + 1) * 0x85EBCA6BU);
}

void FillInput(std::vector<unsigned char>* pBytes, int pRunIndex, int pGameIndex) {
  std::uint32_t aState = SeedState(pRunIndex + static_cast<int>(pBytes->size()), pGameIndex);
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

std::uint64_t HashHistogram(const Histogram256& pHistogram) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::uint64_t aCount : pHistogram.mCounts) {
    aDigest = HashMix(aDigest, aCount);
  }
  return aDigest;
}

std::uint64_t HashBytes(const std::vector<unsigned char>& pBytes) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (unsigned char aByte : pBytes) {
    aDigest = HashMix(aDigest, static_cast<std::uint64_t>(aByte));
  }
  return aDigest;
}

std::uint64_t HashRuntimeStats(const peanutbutter::maze::Maze::RuntimeStats& pStats,
                               const peanutbutter::maze::MazeDirector::ProbeStats& pProbeStats) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  aDigest = HashMix(aDigest, pStats.mVictories);
  aDigest = HashMix(aDigest, pStats.mDeaths);
  aDigest = HashMix(aDigest, pStats.mFlush);
  aDigest = HashMix(aDigest, pStats.mEmptyFlush);
  aDigest = HashMix(aDigest, pStats.mTilesFlushedRegular);
  aDigest = HashMix(aDigest, pStats.mTilesFlushedStalled);
  aDigest = HashMix(aDigest, pStats.mTilesPaintedValidScenario);
  aDigest = HashMix(aDigest, pStats.mInconsistentStateA);
  aDigest = HashMix(aDigest, pStats.mInconsistentStateB);
  aDigest = HashMix(aDigest, pStats.mInconsistentStateC);
  aDigest = HashMix(aDigest, pStats.mInconsistentStateD);
  aDigest = HashMix(aDigest, pStats.mInconsistentStateE);
  aDigest = HashMix(aDigest, pStats.mSimulationStallCataclysmic);
  aDigest = HashMix(aDigest, pStats.mSimulationStallApocalypse);
  aDigest = HashMix(aDigest, pStats.mStarBurst);
  aDigest = HashMix(aDigest, pStats.mChaosStorm);
  aDigest = HashMix(aDigest, pStats.mCometTrailsHorizontal);
  aDigest = HashMix(aDigest, pStats.mCometTrailsVertical);
  aDigest = HashMix(aDigest, pProbeStats.mSimulationCount);
  aDigest = HashMix(aDigest, pProbeStats.mRobotLifeCount);
  aDigest = HashMix(aDigest, pProbeStats.mTotalRobotWalk);
  aDigest = HashMix(aDigest, pProbeStats.mShortestRobotWalk);
  aDigest = HashMix(aDigest, pProbeStats.mLongestRobotWalk);
  aDigest = HashMix(aDigest, pProbeStats.mSharkKillCount);
  aDigest = HashMix(aDigest, pProbeStats.mTotalSharkMovesBeforeKill);
  return aDigest;
}

bool RuntimeStatsEqual(const peanutbutter::maze::Maze::RuntimeStats& pA,
                       const peanutbutter::maze::Maze::RuntimeStats& pB) {
  return pA.mVictories == pB.mVictories &&
         pA.mDeaths == pB.mDeaths &&
         pA.mFlush == pB.mFlush &&
         pA.mEmptyFlush == pB.mEmptyFlush &&
         pA.mTilesFlushedRegular == pB.mTilesFlushedRegular &&
         pA.mTilesFlushedStalled == pB.mTilesFlushedStalled &&
         pA.mTilesPaintedValidScenario == pB.mTilesPaintedValidScenario &&
         pA.mInconsistentStateA == pB.mInconsistentStateA &&
         pA.mInconsistentStateB == pB.mInconsistentStateB &&
         pA.mInconsistentStateC == pB.mInconsistentStateC &&
         pA.mInconsistentStateD == pB.mInconsistentStateD &&
         pA.mInconsistentStateE == pB.mInconsistentStateE &&
         pA.mSimulationStallCataclysmic == pB.mSimulationStallCataclysmic &&
         pA.mSimulationStallApocalypse == pB.mSimulationStallApocalypse &&
         pA.mStarBurst == pB.mStarBurst &&
         pA.mChaosStorm == pB.mChaosStorm &&
         pA.mCometTrailsHorizontal == pB.mCometTrailsHorizontal &&
         pA.mCometTrailsVertical == pB.mCometTrailsVertical;
}

bool ProbeStatsEqual(const peanutbutter::maze::MazeDirector::ProbeStats& pA,
                     const peanutbutter::maze::MazeDirector::ProbeStats& pB) {
  return pA.mSimulationCount == pB.mSimulationCount &&
         pA.mRobotLifeCount == pB.mRobotLifeCount &&
         pA.mTotalRobotWalk == pB.mTotalRobotWalk &&
         pA.mShortestRobotWalk == pB.mShortestRobotWalk &&
         pA.mLongestRobotWalk == pB.mLongestRobotWalk &&
         pA.mSharkKillCount == pB.mSharkKillCount &&
         pA.mTotalSharkMovesBeforeKill == pB.mTotalSharkMovesBeforeKill;
}

bool FindOpenExtremes(const peanutbutter::maze::Maze& pMaze,
                      std::pair<int, int>* pStart,
                      std::pair<int, int>* pEnd,
                      int* pOpenCount) {
  if (pStart == nullptr || pEnd == nullptr || pOpenCount == nullptr) {
    return false;
  }

  bool aFound = false;
  int aOpen = 0;
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
      if (pMaze.IsWall(aX, aY)) {
        continue;
      }
      if (!aFound) {
        *pStart = {aX, aY};
        aFound = true;
      }
      *pEnd = {aX, aY};
      ++aOpen;
    }
  }

  *pOpenCount = aOpen;
  return aFound;
}

bool VerifyConnectivity(peanutbutter::maze::MazeDirector* pMaze, std::uint64_t* pTopologyDigest) {
  if (pMaze == nullptr || pTopologyDigest == nullptr) {
    return false;
  }

  std::pair<int, int> aStart{};
  std::pair<int, int> aEnd{};
  int aOpenCount = 0;
  if (!FindOpenExtremes(*pMaze, &aStart, &aEnd, &aOpenCount)) {
    return false;
  }
  if (!pMaze->FindPath(aStart.first, aStart.second, aEnd.first, aEnd.second)) {
    return false;
  }

  *pTopologyDigest = HashMix(*pTopologyDigest, static_cast<std::uint64_t>(aOpenCount));
  *pTopologyDigest = HashMix(*pTopologyDigest, static_cast<std::uint64_t>(pMaze->PathLength()));
  *pTopologyDigest = HashMix(*pTopologyDigest,
                             static_cast<std::uint64_t>((aStart.first & 0xFF) |
                                                        ((aStart.second & 0xFF) << 8) |
                                                        ((aEnd.first & 0xFF) << 16) |
                                                        ((aEnd.second & 0xFF) << 24)));
  return true;
}

void Accumulate(StabilityAggregate* pAggregate,
                const peanutbutter::maze::Maze::RuntimeStats& pStats,
                const peanutbutter::maze::MazeDirector::ProbeStats& pProbeStats,
                const std::vector<unsigned char>& pOutput,
                std::uint64_t pTopologyDigestContribution) {
  pAggregate->mRuns += 1U;
  pAggregate->mRepeatabilityChecks += 1U;
  pAggregate->mHistogramChecks += 1U;
  pAggregate->mConnectivityChecks += 1U;
  pAggregate->mVictories += pStats.mVictories;
  pAggregate->mDeaths += pStats.mDeaths;
  pAggregate->mFlush += pStats.mFlush;
  pAggregate->mEmptyFlush += pStats.mEmptyFlush;
  pAggregate->mTilesFlushedRegular += pStats.mTilesFlushedRegular;
  pAggregate->mTilesFlushedStalled += pStats.mTilesFlushedStalled;
  pAggregate->mTilesPaintedValidScenario += pStats.mTilesPaintedValidScenario;
  pAggregate->mInconsistentStateA += pStats.mInconsistentStateA;
  pAggregate->mInconsistentStateB += pStats.mInconsistentStateB;
  pAggregate->mInconsistentStateC += pStats.mInconsistentStateC;
  pAggregate->mInconsistentStateD += pStats.mInconsistentStateD;
  pAggregate->mInconsistentStateE += pStats.mInconsistentStateE;
  pAggregate->mSimulationStallCataclysmic += pStats.mSimulationStallCataclysmic;
  pAggregate->mSimulationStallApocalypse += pStats.mSimulationStallApocalypse;
  pAggregate->mStarBurst += pStats.mStarBurst;
  pAggregate->mChaosStorm += pStats.mChaosStorm;
  pAggregate->mCometTrailsHorizontal += pStats.mCometTrailsHorizontal;
  pAggregate->mCometTrailsVertical += pStats.mCometTrailsVertical;
  pAggregate->mSimulationCount += pProbeStats.mSimulationCount;
  pAggregate->mRobotLifeCount += pProbeStats.mRobotLifeCount;
  pAggregate->mTotalRobotWalk += pProbeStats.mTotalRobotWalk;
  if (pProbeStats.mRobotLifeCount > 0U) {
    if (pProbeStats.mShortestRobotWalk < pAggregate->mShortestRobotWalk) {
      pAggregate->mShortestRobotWalk = pProbeStats.mShortestRobotWalk;
    }
    if (pProbeStats.mLongestRobotWalk > pAggregate->mLongestRobotWalk) {
      pAggregate->mLongestRobotWalk = pProbeStats.mLongestRobotWalk;
    }
  }
  pAggregate->mSharkKillCount += pProbeStats.mSharkKillCount;
  pAggregate->mTotalSharkMovesBeforeKill += pProbeStats.mTotalSharkMovesBeforeKill;
  pAggregate->mOutputDigest = HashMix(pAggregate->mOutputDigest, HashBytes(pOutput));
  pAggregate->mRuntimeDigest = HashMix(pAggregate->mRuntimeDigest, HashRuntimeStats(pStats, pProbeStats));
  pAggregate->mTopologyDigest = HashMix(pAggregate->mTopologyDigest, pTopologyDigestContribution);
}

double SafeAverage(std::uint64_t pTotal, std::uint64_t pCount) {
  if (pCount == 0U) {
    return 0.0;
  }
  return static_cast<double>(pTotal) / static_cast<double>(pCount);
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

  std::cout << "# Maze Stability Tests\n";
  std::cout << "# runs_per_maze=" << aRuns << "\n";

  for (int aGameIndex = 0; aGameIndex < peanutbutter::maze::kGameCount; ++aGameIndex) {
    const char* aName = peanutbutter::maze::GetProbeConfigNameByIndex(aGameIndex);
    StabilityAggregate aAggregate;

    std::cout << "[MAZE] " << aGameIndex << " " << aName << "\n";

    for (int aRun = 0; aRun < aRuns; ++aRun) {
      std::vector<unsigned char> aInput(64U, 0U);
      FillInput(&aInput, aRun, aGameIndex);
      const std::vector<unsigned char> aSeed = peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(
          aInput.data(), static_cast<int>(aInput.size()), peanutbutter::maze::Maze::kSeedBufferCapacity);
      const Histogram256 aSeedHistogram = BuildHistogram(aSeed);

      peanutbutter::maze::MazeDirector aMazeA;
      aMazeA.SetGame(aGameIndex);
      aMazeA.Seed(const_cast<unsigned char*>(aSeed.data()), static_cast<int>(aSeed.size()));
      std::vector<unsigned char> aOutputA(aSeed.size(), 0U);
      aMazeA.Get(aOutputA.data(), static_cast<int>(aOutputA.size()));
      const Histogram256 aOutputHistogramA = BuildHistogram(aOutputA);
      const peanutbutter::maze::Maze::RuntimeStats aStatsA = aMazeA.GetRuntimeStats();
      const peanutbutter::maze::MazeDirector::ProbeStats aProbeStatsA = aMazeA.GetProbeStats();
      std::uint64_t aTopologyDigestA = 1469598103934665603ULL;
      if (!VerifyConnectivity(&aMazeA, &aTopologyDigestA)) {
        ++aAggregate.mConnectivityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " connectivity check failed at run " << aRun << "\n";
        continue;
      }

      peanutbutter::maze::MazeDirector aMazeB;
      aMazeB.SetGame(aGameIndex);
      aMazeB.Seed(const_cast<unsigned char*>(aSeed.data()), static_cast<int>(aSeed.size()));
      std::vector<unsigned char> aOutputB(aSeed.size(), 0U);
      aMazeB.Get(aOutputB.data(), static_cast<int>(aOutputB.size()));
      const Histogram256 aOutputHistogramB = BuildHistogram(aOutputB);
      const peanutbutter::maze::Maze::RuntimeStats aStatsB = aMazeB.GetRuntimeStats();
      const peanutbutter::maze::MazeDirector::ProbeStats aProbeStatsB = aMazeB.GetProbeStats();
      std::uint64_t aTopologyDigestB = 1469598103934665603ULL;
      if (!VerifyConnectivity(&aMazeB, &aTopologyDigestB)) {
        ++aAggregate.mConnectivityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " repeat connectivity check failed at run " << aRun << "\n";
        continue;
      }

      if (aOutputA != aOutputB) {
        ++aAggregate.mRepeatabilityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " output repeatability mismatch at run " << aRun << "\n";
        continue;
      }
      if (!RuntimeStatsEqual(aStatsA, aStatsB) || !ProbeStatsEqual(aProbeStatsA, aProbeStatsB)) {
        ++aAggregate.mRuntimeRepeatabilityFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " runtime repeatability mismatch at run " << aRun << "\n";
        continue;
      }
      if (!HistogramsMatch(aSeedHistogram, aOutputHistogramA) || !HistogramsMatch(aOutputHistogramA, aOutputHistogramB)) {
        ++aAggregate.mHistogramDriftCount;
        std::cerr << "[WARN] " << aName << " histogram drift at run " << aRun
                  << " seed_hist=" << HashHistogram(aSeedHistogram)
                  << " output_hist=" << HashHistogram(aOutputHistogramA) << "\n";
      }
      if (aTopologyDigestA != aTopologyDigestB) {
        ++aAggregate.mTopologyFailures;
        aHasHardFailure = true;
        std::cerr << "[FAIL] " << aName << " topology repeatability mismatch at run " << aRun << "\n";
        continue;
      }

      Accumulate(&aAggregate, aStatsA, aProbeStatsA, aOutputA, aTopologyDigestA);
    }

    const std::uint64_t aShortestRobotWalk =
        (aAggregate.mShortestRobotWalk == std::numeric_limits<std::uint64_t>::max()) ? 0U
                                                                                      : aAggregate.mShortestRobotWalk;

    std::cout << "  runs=" << aAggregate.mRuns
              << " repeatability_checks=" << aAggregate.mRepeatabilityChecks
              << " histogram_checks=" << aAggregate.mHistogramChecks
              << " connectivity_checks=" << aAggregate.mConnectivityChecks << "\n";
    std::cout << "  repeatability_failures=" << aAggregate.mRepeatabilityFailures
              << " runtime_repeatability_failures=" << aAggregate.mRuntimeRepeatabilityFailures
              << " histogram_drift=" << aAggregate.mHistogramDriftCount
              << " connectivity_failures=" << aAggregate.mConnectivityFailures
              << " topology_failures=" << aAggregate.mTopologyFailures << "\n";
    std::cout << "  victories=" << aAggregate.mVictories
              << " deaths=" << aAggregate.mDeaths
              << " flush=" << aAggregate.mFlush
              << " empty_flush=" << aAggregate.mEmptyFlush << "\n";
    std::cout << "  tiles_painted=" << aAggregate.mTilesPaintedValidScenario
              << " tiles_flushed_regular=" << aAggregate.mTilesFlushedRegular
              << " tiles_flushed_stalled=" << aAggregate.mTilesFlushedStalled << "\n";
    std::cout << "  inconsistent_a=" << aAggregate.mInconsistentStateA
              << " inconsistent_b=" << aAggregate.mInconsistentStateB
              << " inconsistent_c=" << aAggregate.mInconsistentStateC
              << " inconsistent_d=" << aAggregate.mInconsistentStateD
              << " inconsistent_e=" << aAggregate.mInconsistentStateE
              << " stall_cataclysmic=" << aAggregate.mSimulationStallCataclysmic
              << " stall_apocalypse=" << aAggregate.mSimulationStallApocalypse << "\n";
    std::cout << "  star_burst=" << aAggregate.mStarBurst
              << " chaos_storm=" << aAggregate.mChaosStorm
              << " comet_horizontal=" << aAggregate.mCometTrailsHorizontal
              << " comet_vertical=" << aAggregate.mCometTrailsVertical << "\n";
    std::cout << "  simulation_count=" << aAggregate.mSimulationCount
              << " robot_lives=" << aAggregate.mRobotLifeCount
              << " avg_robot_walk=" << std::fixed << std::setprecision(4)
              << SafeAverage(aAggregate.mTotalRobotWalk, aAggregate.mRobotLifeCount)
              << " shortest_robot_walk=" << aShortestRobotWalk
              << " longest_robot_walk=" << aAggregate.mLongestRobotWalk << "\n";
    std::cout << "  shark_kills=" << aAggregate.mSharkKillCount
              << " avg_shark_moves_before_kill=" << std::fixed << std::setprecision(4)
              << SafeAverage(aAggregate.mTotalSharkMovesBeforeKill, aAggregate.mSharkKillCount) << "\n";
    std::cout << "  output_digest=" << aAggregate.mOutputDigest
              << " runtime_digest=" << aAggregate.mRuntimeDigest
              << " topology_digest=" << aAggregate.mTopologyDigest << "\n";
  }

  if (aHasHardFailure) {
    std::cerr << "[FAIL] maze stability tests found hard stability failures\n";
    return 1;
  }

  std::cout << "[PASS] maze stability tests passed\n";
  return 0;
}
