#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/Tables/counters/MersenneCounter.hpp"
#include "src/Tables/maze/Maze.hpp"
#include "src/Tables/rng/Counter.hpp"
#include "tests/common/CounterSeedBuffer.hpp"
#include "tests/common/Tests.hpp"

namespace {

class MazeQualityHarness final : public peanutbutter::maze::Maze {
 public:
  using peanutbutter::maze::Maze::Get;

  void Seed(unsigned char* pPassword, int pPasswordLength) override {
    InitializeSeedBuffer(pPassword, pPasswordLength);
    mIndexState = 0x811C9DC5U;
    if (pPassword == nullptr || pPasswordLength <= 0) {
      return;
    }
    for (int aIndex = 0; aIndex < pPasswordLength; ++aIndex) {
      mIndexState ^= static_cast<std::uint32_t>(pPassword[aIndex]);
      mIndexState *= 16777619U;
    }
  }

 protected:
  int NextIndex(int pLimit) override {
    if (pLimit <= 1) {
      return 0;
    }
    mIndexState ^= (mIndexState << 13U);
    mIndexState ^= (mIndexState >> 17U);
    mIndexState ^= (mIndexState << 5U);
    return static_cast<int>(mIndexState % static_cast<std::uint32_t>(pLimit));
  }

 public:
  void BuildPrims() {
    GeneratePrims();
    FinalizeWalls();
  }

  void BuildKruskals() {
    ExecuteKruskals();
    FinalizeWalls();
  }

  void FillRandomWalls(peanutbutter::rng::Counter* pCounter, int pWallDivisor) {
    ClearWalls();
    for (int aY = 0; aY < kGridHeight; ++aY) {
      for (int aX = 0; aX < kGridWidth; ++aX) {
        const bool aIsWall =
            (pCounter != nullptr && pWallDivisor > 0) ? ((pCounter->Get() % static_cast<unsigned char>(pWallDivisor)) == 0U)
                                                      : false;
        SetWall(aX, aY, aIsWall);
      }
    }
  }

  void OpenCell(int pX, int pY) {
    SetWall(pX, pY, false);
  }

 private:
  std::uint32_t mIndexState = 0x811C9DC5U;
};

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pSalt);
  return 0xB5297A4DU ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x68E31DA4U);
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

int Distance1(int pX1, int pY1, int pX2, int pY2) {
  return std::abs(pX1 - pX2) + std::abs(pY1 - pY2);
}

bool FindFirstOpenCell(MazeQualityHarness* pMaze, int* pOutX, int* pOutY) {
  if (pMaze == nullptr || pOutX == nullptr || pOutY == nullptr) {
    return false;
  }
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
      if (!pMaze->IsWall(aX, aY)) {
        *pOutX = aX;
        *pOutY = aY;
        return true;
      }
    }
  }
  return false;
}

bool VerifyGeneratedMaze(MazeQualityHarness* pMaze,
                         const char* pName,
                         int pLoop,
                         int* pOutOpenCount,
                         int* pOutEndX,
                         int* pOutEndY) {
  if (pMaze == nullptr) {
    std::cerr << "[FAIL] " << pName << " generator produced null maze at loop " << pLoop << "\n";
    return false;
  }

  int aStartX = -1;
  int aStartY = -1;
  if (!FindFirstOpenCell(pMaze, &aStartX, &aStartY)) {
    std::cerr << "[FAIL] " << pName << " generator produced no open tiles at loop " << pLoop << "\n";
    return false;
  }

  bool aSeen[peanutbutter::maze::Maze::kGridWidth][peanutbutter::maze::Maze::kGridHeight] = {};
  int aStackX[peanutbutter::maze::Maze::kGridSize];
  int aStackY[peanutbutter::maze::Maze::kGridSize];
  int aStackCount = 0;
  int aLastX = aStartX;
  int aLastY = aStartY;
  aSeen[aStartX][aStartY] = true;
  aStackX[aStackCount] = aStartX;
  aStackY[aStackCount] = aStartY;
  ++aStackCount;

  while (aStackCount > 0) {
    --aStackCount;
    const int aX = aStackX[aStackCount];
    const int aY = aStackY[aStackCount];
    aLastX = aX;
    aLastY = aY;

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};
    for (int aDir = 0; aDir < 4; ++aDir) {
      if (aNX[aDir] < 0 || aNX[aDir] >= peanutbutter::maze::Maze::kGridWidth || aNY[aDir] < 0 ||
          aNY[aDir] >= peanutbutter::maze::Maze::kGridHeight || aSeen[aNX[aDir]][aNY[aDir]] ||
          pMaze->IsWall(aNX[aDir], aNY[aDir])) {
        continue;
      }
      aSeen[aNX[aDir]][aNY[aDir]] = true;
      aStackX[aStackCount] = aNX[aDir];
      aStackY[aStackCount] = aNY[aDir];
      ++aStackCount;
    }
  }

  int aOpenCount = 0;
  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
      if (pMaze->IsWall(aX, aY)) {
        continue;
      }
      ++aOpenCount;
      if (!aSeen[aX][aY]) {
        std::cerr << "[FAIL] " << pName << " generator left unreachable tile at loop " << pLoop
                  << " x=" << aX << " y=" << aY << "\n";
        return false;
      }
    }
  }

  if (!pMaze->FindPath(aStartX, aStartY, aLastX, aLastY) || pMaze->PathLength() <= 0) {
    std::cerr << "[FAIL] " << pName << " generator path check failed at loop " << pLoop << "\n";
    return false;
  }

  if (pOutOpenCount != nullptr) {
    *pOutOpenCount = aOpenCount;
  }
  if (pOutEndX != nullptr) {
    *pOutEndX = aLastX;
  }
  if (pOutEndY != nullptr) {
    *pOutEndY = aLastY;
  }
  return true;
}

}  // namespace

int main() {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] maze pathfinding quality disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aSeedLength =
      (peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::MAZE_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;
  std::cout << "[INFO] maze pathfinding quality"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " trials_per_loop=3"
            << " global_seed=" << peanutbutter::tests::config::TEST_SEED_GLOBAL << "\n";

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    constexpr int kWallDivisors[3] = {8, 4, 2};
    constexpr const char* kTrialNames[3] = {"sparse", "medium", "dense"};

    for (int aTrialKind = 0; aTrialKind < 3; ++aTrialKind) {
      std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
      FillInput(&aInput, (aLoop * 3) + aTrialKind);
      const std::vector<unsigned char> aSeed =
          peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(aInput.data(), aSeedLength, aSeedLength);

      MazeQualityHarness aMaze;
      aMaze.Seed(const_cast<unsigned char*>(aSeed.data()), aSeedLength);

      MersenneCounter aCounter;
      aCounter.Seed(aInput.data(), aSeedLength);
      aMaze.FillRandomWalls(&aCounter, kWallDivisors[aTrialKind]);

      const int aStartX = static_cast<int>(aCounter.Get() % peanutbutter::maze::Maze::kGridWidth);
      const int aStartY = static_cast<int>(aCounter.Get() % peanutbutter::maze::Maze::kGridHeight);
      int aEndX = static_cast<int>(aCounter.Get() % peanutbutter::maze::Maze::kGridWidth);
      int aEndY = static_cast<int>(aCounter.Get() % peanutbutter::maze::Maze::kGridHeight);
      if (aStartX == aEndX && aStartY == aEndY) {
        aEndX = (aEndX + 1) % peanutbutter::maze::Maze::kGridWidth;
      }

      aMaze.OpenCell(aStartX, aStartY);
      aMaze.OpenCell(aEndX, aEndY);

      const bool aConnectedSlow = aMaze.IsConnected_Slow(aStartX, aStartY, aEndX, aEndY);
      const bool aConnectedFast = aMaze.FindPath(aStartX, aStartY, aEndX, aEndY);
      if (aConnectedSlow != aConnectedFast) {
        std::cerr << "[FAIL] maze connectivity mismatch at loop " << aLoop
                  << " trial=" << kTrialNames[aTrialKind]
                  << " start=(" << aStartX << "," << aStartY << ")"
                  << " end=(" << aEndX << "," << aEndY << ")"
                  << " slow=" << static_cast<int>(aConnectedSlow)
                  << " fast=" << static_cast<int>(aConnectedFast) << "\n";
        return 1;
      }

      if (aConnectedFast) {
        if (aMaze.PathLength() <= 0) {
          std::cerr << "[FAIL] maze path unexpectedly empty at loop " << aLoop
                    << " trial=" << kTrialNames[aTrialKind] << "\n";
          return 1;
        }

        int aPrevX = -1;
        int aPrevY = -1;
        for (int aIndex = 0; aIndex < aMaze.PathLength(); ++aIndex) {
          int aPathX = -1;
          int aPathY = -1;
          if (!aMaze.PathNode(aIndex, &aPathX, &aPathY)) {
            std::cerr << "[FAIL] maze path node missing at loop " << aLoop
                      << " trial=" << kTrialNames[aTrialKind]
                      << " index=" << aIndex << "\n";
            return 1;
          }
          if (aMaze.IsWall(aPathX, aPathY)) {
            std::cerr << "[FAIL] maze path walks through wall at loop " << aLoop
                      << " trial=" << kTrialNames[aTrialKind]
                      << " x=" << aPathX << " y=" << aPathY << "\n";
            return 1;
          }
          if (aIndex == 0 && (aPathX != aStartX || aPathY != aStartY)) {
            std::cerr << "[FAIL] maze path start mismatch at loop " << aLoop
                      << " trial=" << kTrialNames[aTrialKind] << "\n";
            return 1;
          }
          if (aIndex > 0 && Distance1(aPrevX, aPrevY, aPathX, aPathY) != 1) {
            std::cerr << "[FAIL] maze path step is not adjacent at loop " << aLoop
                      << " trial=" << kTrialNames[aTrialKind]
                      << " index=" << aIndex << "\n";
            return 1;
          }
          aPrevX = aPathX;
          aPrevY = aPathY;
        }
        if (aPrevX != aEndX || aPrevY != aEndY) {
          std::cerr << "[FAIL] maze path end mismatch at loop " << aLoop
                    << " trial=" << kTrialNames[aTrialKind] << "\n";
          return 1;
        }
      }

      aDigest ^= static_cast<std::uint64_t>(aConnectedFast ? 1U : 0U);
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aMaze.PathLength());
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aStartX + (aStartY << 8));
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aEndX + (aEndY << 8));
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(kWallDivisors[aTrialKind]);
    }

    {
      std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
      FillInput(&aInput, (aLoops * 3) + aLoop);
      const std::vector<unsigned char> aSeed =
          peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(aInput.data(), aSeedLength, aSeedLength);

      MazeQualityHarness aPrimsMaze;
      aPrimsMaze.Seed(const_cast<unsigned char*>(aSeed.data()), aSeedLength);
      aPrimsMaze.BuildPrims();
      int aOpenCount = 0;
      int aEndX = -1;
      int aEndY = -1;
      if (!VerifyGeneratedMaze(&aPrimsMaze, "prims", aLoop, &aOpenCount, &aEndX, &aEndY)) {
        return 1;
      }
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aOpenCount);
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aPrimsMaze.PathLength());
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aEndX + (aEndY << 8));
    }

    {
      std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
      FillInput(&aInput, (aLoops * 4) + aLoop);
      const std::vector<unsigned char> aSeed =
          peanutbutter::tests::BuildCounterSeedBuffer<MersenneCounter>(aInput.data(), aSeedLength, aSeedLength);

      MazeQualityHarness aKruskalsMaze;
      aKruskalsMaze.Seed(const_cast<unsigned char*>(aSeed.data()), aSeedLength);
      aKruskalsMaze.BuildKruskals();
      int aOpenCount = 0;
      int aEndX = -1;
      int aEndY = -1;
      if (!VerifyGeneratedMaze(&aKruskalsMaze, "kruskals", aLoop, &aOpenCount, &aEndX, &aEndY)) {
        return 1;
      }
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aOpenCount);
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aKruskalsMaze.PathLength());
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>((aEndX << 8) + aEndY);
    }
  }

  std::cout << "[PASS] maze pathfinding quality passed"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " digest=" << aDigest << "\n";
  return 0;
}
