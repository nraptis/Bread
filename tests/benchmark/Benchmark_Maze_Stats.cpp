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
  return 0xA24BAED5U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
}

void FillInput(std::vector<unsigned char>* pBytes, int pSalt) {
  std::uint32_t aState = SeedState(pSalt + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

template <typename TMaze>
bool RunOne(const char* pName, int pInputLength, int pSalt, std::uint64_t* pDigest) {
  std::vector<unsigned char> aInput(static_cast<std::size_t>(pInputLength), 0U);
  std::vector<unsigned char> aOutput(static_cast<std::size_t>(pInputLength), 0U);
  FillInput(&aInput, pSalt);

  MersenneCounter aCounter;
  TMaze aMaze(nullptr, &aCounter);
  aMaze.Seed(aInput.data(), pInputLength);
  aMaze.Get(aOutput.data(), pInputLength);

  if (pDigest != nullptr) {
    for (unsigned char aByte : aOutput) {
      *pDigest = (*pDigest * 1315423911ULL) ^ static_cast<std::uint64_t>(aByte);
    }
  }

  const bread::maze::Maze::RuntimeStats aStats = aMaze.GetRuntimeStats();
  std::cout << "[STAT] " << pName
            << " runs=1"
            << " bytes=" << pInputLength
            << " is_robot=" << aStats.mIsRobot
            << " is_dolphin=" << aStats.mIsDolphin
            << " flush=" << aStats.mFlush
            << " empty_flush=" << aStats.mEmptyFlush << "\n";
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
  std::uint64_t aDigest = 1469598103934665603ULL;

  if (!RunOne<bread::maze::MazeRobotCheese>("MazeRobotCheese", aInputLength, 11, &aDigest) ||
      !RunOne<bread::maze::MazeDolphinSharks>("MazeDolphinSharks", aInputLength, 23, &aDigest)) {
    return 1;
  }

  std::cout << "[PASS] maze stats benchmark passed"
            << " runs_per_maze=1"
            << " bytes=" << aInputLength
            << " digest=" << aDigest << "\n";
  return 0;
}
