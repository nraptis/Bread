#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/counters/MersenneCounter.hpp"
#include "src/maze/Maze.hpp"
#include "src/rng/Counter.hpp"
#include "tests/common/Tests.hpp"

namespace {

class MazeQualityHarness final : public bread::maze::Maze {
 public:
  using bread::maze::Maze::Get;

  void Seed(unsigned char* pPassword, int pPasswordLength) override {
    InitializeSeedBuffer(pPassword, pPasswordLength, nullptr);
  }

  void FillRandomWalls(bread::rng::Counter* pCounter, int pWallDivisor) {
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
};

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
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

}  // namespace

int main() {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] maze pathfinding quality disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aSeedLength =
      (bread::tests::config::MAZE_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::MAZE_TEST_DATA_LENGTH;

  std::uint64_t aDigest = 1469598103934665603ULL;
  std::cout << "[INFO] maze pathfinding quality"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " trials_per_loop=3"
            << " global_seed=" << bread::tests::config::TEST_SEED_GLOBAL << "\n";

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    constexpr int kWallDivisors[3] = {8, 4, 2};
    constexpr const char* kTrialNames[3] = {"sparse", "medium", "dense"};

    for (int aTrialKind = 0; aTrialKind < 3; ++aTrialKind) {
      std::vector<unsigned char> aInput(static_cast<std::size_t>(aSeedLength), 0U);
      FillInput(&aInput, (aLoop * 3) + aTrialKind);

      MazeQualityHarness aMaze;
      aMaze.Seed(aInput.data(), aSeedLength);

      MersenneCounter aCounter;
      aCounter.Seed(aInput.data(), aSeedLength);
      aMaze.FillRandomWalls(&aCounter, kWallDivisors[aTrialKind]);

      const int aStartX = static_cast<int>(aCounter.Get() % bread::maze::Maze::kGridWidth);
      const int aStartY = static_cast<int>(aCounter.Get() % bread::maze::Maze::kGridHeight);
      int aEndX = static_cast<int>(aCounter.Get() % bread::maze::Maze::kGridWidth);
      int aEndY = static_cast<int>(aCounter.Get() % bread::maze::Maze::kGridHeight);
      if (aStartX == aEndX && aStartY == aEndY) {
        aEndX = (aEndX + 1) % bread::maze::Maze::kGridWidth;
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
  }

  std::cout << "[PASS] maze pathfinding quality passed"
            << " loops=" << aLoops
            << " bytes=" << aSeedLength
            << " digest=" << aDigest << "\n";
  return 0;
}
