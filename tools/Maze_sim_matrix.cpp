#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/maze/MazePolicy.hpp"
#include "tests/common/CounterSeedBuffer.hpp"

namespace {

struct AggregateStats {
  std::uint64_t mSimulationCount = 0U;
  std::uint64_t mVictories = 0U;
  std::uint64_t mDeaths = 0U;
  std::uint64_t mRobotLifeCount = 0U;
  std::uint64_t mTotalRobotWalk = 0U;
  std::uint64_t mShortestRobotWalk = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t mLongestRobotWalk = 0U;
  std::uint64_t mSharkKillCount = 0U;
  std::uint64_t mTotalSharkMovesBeforeKill = 0U;
};

const char* GenerationName(peanutbutter::maze::GenerationMode pMode) {
  switch (pMode) {
    case peanutbutter::maze::GenerationMode::kCustom:
      return "custom";
    case peanutbutter::maze::GenerationMode::kPrim:
      return "prim";
    case peanutbutter::maze::GenerationMode::kKruskal:
      return "kruskal";
    default:
      return "random";
  }
}

std::uint32_t SeedState(int pRunIndex,
                        int pGameIndex,
                        peanutbutter::maze::GenerationMode pMode,
                        int pRobots,
                        int pSharks,
                        int pCheeses) {
  const std::uint32_t aMode = static_cast<std::uint32_t>(static_cast<int>(pMode) + 7);
  return 0xA5B3571DU ^ (static_cast<std::uint32_t>(pRunIndex) * 0x9E3779B9U) ^
         (static_cast<std::uint32_t>(pGameIndex) << 1U) ^
         (static_cast<std::uint32_t>(pRobots) << 4U) ^ (static_cast<std::uint32_t>(pSharks) << 10U) ^
         (static_cast<std::uint32_t>(pCheeses) << 16U) ^ (aMode << 24U);
}

void FillInput(std::vector<unsigned char>* pBytes,
               int pRunIndex,
               int pGameIndex,
               peanutbutter::maze::GenerationMode pMode,
               int pRobots,
               int pSharks,
               int pCheeses) {
  std::uint32_t aState = SeedState(pRunIndex, pGameIndex, pMode, pRobots, pSharks, pCheeses);
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

void Accumulate(AggregateStats* pAggregate,
                const peanutbutter::maze::Maze::RuntimeStats& pRuntimeStats,
                const peanutbutter::maze::MazeDirector::ProbeStats& pProbeStats) {
  pAggregate->mSimulationCount += pProbeStats.mSimulationCount;
  pAggregate->mVictories += pRuntimeStats.mVictories;
  pAggregate->mDeaths += pRuntimeStats.mDeaths;
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
}

double SafeAverage(std::uint64_t pTotal, std::uint64_t pCount) {
  if (pCount == 0U) {
    return 0.0;
  }
  return static_cast<double>(pTotal) / static_cast<double>(pCount);
}

}  // namespace

int main(int argc, char** argv) {
  int aRuns = 1000;
  if (argc >= 2) {
    aRuns = std::atoi(argv[1]);
  }
  if (aRuns <= 0) {
    std::cerr << "[FAIL] run count must be positive\n";
    return 1;
  }

  std::cout << "# Maze Simulation Matrix\n";
  std::cout << "# runs_per_case=" << aRuns << "\n";
  std::cout << "game_index,name,generation,robots,sharks,cheeses,runs,trials,victories,deaths,longest_robot_walk,"
               "shortest_robot_walk,average_robot_walk,average_shark_moves_before_kill,shark_kills\n";

  for (int aGameIndex = 0; aGameIndex < peanutbutter::maze::kGameCount; ++aGameIndex) {
    const peanutbutter::maze::ProbeConfig& aConfig = peanutbutter::maze::GetProbeConfigByIndex(aGameIndex);
    AggregateStats aAggregate;
    for (int aRun = 0; aRun < aRuns; ++aRun) {
      std::vector<unsigned char> aInput(64U, 0U);
      FillInput(&aInput, aRun, aGameIndex, aConfig.mGenerationMode, aConfig.mRobotCount, aConfig.mSharkCount,
                aConfig.mCheeseCount);
      const std::vector<unsigned char> aSeed = peanutbutter::tests::BuildCounterSeedBuffer<ChaCha20Counter>(
          aInput.data(), static_cast<int>(aInput.size()), peanutbutter::maze::Maze::kSeedBufferCapacity);

      peanutbutter::maze::MazeDirector aMaze;
      aMaze.SetGame(aGameIndex);
      aMaze.Seed(const_cast<unsigned char*>(aSeed.data()), static_cast<int>(aSeed.size()));

      Accumulate(&aAggregate, aMaze.GetRuntimeStats(), aMaze.GetProbeStats());
    }

    const std::uint64_t aShortest =
        (aAggregate.mShortestRobotWalk == std::numeric_limits<std::uint64_t>::max()) ? 0U
                                                                                      : aAggregate.mShortestRobotWalk;
    std::cout << aGameIndex << ","
              << peanutbutter::maze::GetProbeConfigNameByIndex(aGameIndex) << ","
              << GenerationName(aConfig.mGenerationMode) << ","
              << aConfig.mRobotCount << ","
              << aConfig.mSharkCount << ","
              << aConfig.mCheeseCount << ","
              << aRuns << ","
              << aAggregate.mSimulationCount << ","
              << aAggregate.mVictories << ","
              << aAggregate.mDeaths << ","
              << aAggregate.mLongestRobotWalk << ","
              << aShortest << ","
              << std::fixed << std::setprecision(4)
              << SafeAverage(aAggregate.mTotalRobotWalk, aAggregate.mRobotLifeCount) << ","
              << SafeAverage(aAggregate.mTotalSharkMovesBeforeKill, aAggregate.mSharkKillCount) << ","
              << aAggregate.mSharkKillCount << "\n";
  }

  return 0;
}
