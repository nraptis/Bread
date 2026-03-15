#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/MazeDolphinSharks.hpp"
#include "src/maze/MazeRobotCheese.hpp"
#include "tests/common/Tests.hpp"

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

template <typename TMaze>
bool RunPerseveranceCase(const char* pName, int pLoops, int pDataLength, int pSalt, std::uint64_t* pDigest) {
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aSeed(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutput(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pSalt);

    MersenneCounter aCounter;
    TMaze aMaze(nullptr, &aCounter);
    aMaze.Seed(aInput.data(), pDataLength);
    aMaze.Get(aOutput.data(), pDataLength);

    MersenneCounter aSeedCounter;
    aSeedCounter.Seed(aInput.data(), pDataLength);
    aSeedCounter.Get(aSeed.data(), pDataLength);

    if (aSeed != aOutput) {
      std::cerr << "[FAIL] " << pName << " perseverance mismatch at loop " << aLoop
                << " bytes=" << pDataLength << "\n";
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

}  // namespace

int main() {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] perseverance disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (bread::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::MAZE_TEST_DATA_LENGTH;
  std::uint64_t aDigest = 1469598103934665603ULL;

  std::cout << "[INFO] maze perseverance tests"
            << " loops=" << aLoops
            << " bytes=" << aDataLength << "\n";

  if (!RunPerseveranceCase<bread::maze::MazeRobotCheese>("MazeRobotCheese", aLoops, aDataLength, 11, &aDigest) ||
      !RunPerseveranceCase<bread::maze::MazeDolphinSharks>("MazeDolphinSharks", aLoops, aDataLength, 23, &aDigest)) {
    return 1;
  }

  std::cout << "[PASS] maze perseverance tests passed"
            << " loops=" << aLoops
            << " bytes=" << aDataLength
            << " digest=" << aDigest << "\n";
  return 0;
}
