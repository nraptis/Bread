#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/Tables/counters/MersenneCounter.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/maze/MazePolicy.hpp"
#include "tests/common/CounterSeedBuffer.hpp"
#include "tests/common/Tests.hpp"

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

bool RunPerseveranceCase(int pGameIndex, int pLoops, int pDataLength, int pSalt, std::uint64_t* pDigest) {
  const char* pName = peanutbutter::maze::GetProbeConfigNameByIndex(pGameIndex);
  for (int aLoop = 0; aLoop < pLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputA(static_cast<std::size_t>(pDataLength), 0U);
    std::vector<unsigned char> aOutputB(static_cast<std::size_t>(pDataLength), 0U);
    FillInput(&aInput, aLoop, pSalt);
    const std::vector<unsigned char> aSeed =
        peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(aInput.data(), pDataLength, pDataLength);

    peanutbutter::maze::MazeDirector aMazeA;
    aMazeA.SetGame(pGameIndex);
    aMazeA.Seed(const_cast<unsigned char*>(aSeed.data()), pDataLength);
    aMazeA.Get(aOutputA.data(), pDataLength);

    peanutbutter::maze::MazeDirector aMazeB;
    aMazeB.SetGame(pGameIndex);
    aMazeB.Seed(const_cast<unsigned char*>(aSeed.data()), pDataLength);
    aMazeB.Get(aOutputB.data(), pDataLength);

    if (aOutputA != aOutputB) {
      std::cerr << "[FAIL] " << pName << " perseverance repeatability mismatch at loop " << aLoop
                << " bytes=" << pDataLength << "\n";
      return false;
    }

    if (pDigest != nullptr) {
      *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pGameIndex);
      for (unsigned char aByte : aOutputA) {
        *pDigest = (*pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
      }
    }
  }

  std::cout << "[PASS] " << pName << " perseverance loops=" << pLoops << " bytes=" << pDataLength << "\n";
  return true;
}

}  // namespace

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] perseverance disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH;
  std::uint64_t aDigest = 1469598103934665603ULL;

  std::cout << "[INFO] maze perseverance tests"
            << " loops=" << aLoops
            << " bytes=" << aDataLength << "\n";

  for (int aGameIndex = 0; aGameIndex < peanutbutter::maze::kGameCount; ++aGameIndex) {
    if (!RunPerseveranceCase(aGameIndex, aLoops, aDataLength, 11 + (aGameIndex * 17), &aDigest)) {
      return 1;
    }
  }

  std::cout << "[PASS] maze perseverance tests passed"
            << " loops=" << aLoops
            << " bytes=" << aDataLength
            << " digest=" << aDigest << "\n";
  return 0;
}
