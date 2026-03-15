#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/Maze.hpp"
#include "src/maze/MazeDolphinSharks.hpp"
#include "src/maze/MazeRobotCheese.hpp"
#include "tests/common/Tests.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0xC2B2AE35U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x27D4EB2FU);
}

void FillInput(std::vector<unsigned char>* pBytes, int pTrial) {
  std::uint32_t aState = SeedState(pTrial + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

template <typename TMaze>
bool RunOne(const char* pName,
            int pInputLength,
            int pTrialsPerMaze,
            std::uint64_t* pDigest,
            std::uint64_t* pRobotTotal,
            std::uint64_t* pDolphinTotal) {
  std::cout << "[RUN] " << pName << " trials=" << pTrialsPerMaze << " bytes=" << pInputLength << std::endl;

  auto aStart = std::chrono::steady_clock::now();
  std::uint64_t aRobot = 0U;
  std::uint64_t aDolphin = 0U;

  for (int aTrial = 0; aTrial < pTrialsPerMaze; ++aTrial) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pInputLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pInputLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pInputLength), 0U);
    FillInput(&aInput, aTrial);

    MersenneCounter aCounterA;
    TMaze aMazeA(nullptr, &aCounterA);
    aMazeA.Seed(aInput.data(), pInputLength);
    aMazeA.Get(aOutputA.data(), pInputLength);

    MersenneCounter aCounterB;
    TMaze aMazeB(nullptr, &aCounterB);
    aMazeB.Seed(aInput.data(), pInputLength);
    aMazeB.Get(aOutputB.data(), pInputLength);

    if (aOutputA != aOutputB) {
      std::cerr << "[FAIL] repeatability mismatch in " << pName << ", trial=" << aTrial << "\n";
      return false;
    }

    if (pDigest != nullptr) {
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1315423911ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }

    const bread::maze::Maze::RuntimeStats aStats = aMazeA.GetRuntimeStats();
    aRobot += aStats.mIsRobot;
    aDolphin += aStats.mIsDolphin;
  }

  auto aEnd = std::chrono::steady_clock::now();
  const auto aMicros = std::chrono::duration_cast<std::chrono::microseconds>(aEnd - aStart).count();
  if (pRobotTotal != nullptr) {
    *pRobotTotal += aRobot;
  }
  if (pDolphinTotal != nullptr) {
    *pDolphinTotal += aDolphin;
  }

  std::cout << "[BENCH] " << pName
            << " trials=" << pTrialsPerMaze
            << " bytes=" << pInputLength
            << " elapsed_us=" << aMicros
            << " is_robot=" << aRobot
            << " is_dolphin=" << aDolphin << "\n";
  return true;
}

}  // namespace

int main() {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const int aInputLength =
      (bread::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::MAZE_TEST_DATA_LENGTH;
  const int aTrialsPerMaze =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;

  std::cout << "[INFO] benchmark_maze_speed_repeatability"
            << " loops=" << aTrialsPerMaze
            << " data_length=" << aInputLength
            << " benchmark_enabled=" << (bread::tests::config::BENCHMARK_ENABLED ? 1 : 0)
            << std::endl;

  std::uint64_t aDigest = 1469598103934665603ULL;
  std::uint64_t aRobotTotal = 0U;
  std::uint64_t aDolphinTotal = 0U;

  if (!RunOne<bread::maze::MazeRobotCheese>(
          "MazeRobotCheese", aInputLength, aTrialsPerMaze, &aDigest, &aRobotTotal, &aDolphinTotal) ||
      !RunOne<bread::maze::MazeDolphinSharks>(
          "MazeDolphinSharks", aInputLength, aTrialsPerMaze, &aDigest, &aRobotTotal, &aDolphinTotal)) {
    return 1;
  }

  std::cout << "[PASS] maze speed/repeatability benchmark passed"
            << " loops=" << aTrialsPerMaze
            << " bytes=" << aInputLength
            << " digest=" << aDigest
            << " is_robot_total=" << aRobotTotal
            << " is_dolphin_total=" << aDolphinTotal << "\n";
  return 0;
}
