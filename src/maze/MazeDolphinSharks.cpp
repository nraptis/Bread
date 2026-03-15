#include "src/maze/MazeDolphinSharks.hpp"

#include <cstring>
#include <utility>

namespace bread::maze {

MazeDolphinSharks::MazeDolphinSharks(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
                                     bread::rng::Counter* pCounter)
    : Maze(),
      mCounter(pCounter),
      mFastRand(pPasswordExpander, bread::fast_rand::FastRandWrapMode::kWrapXOR),
      mStackX{},
      mStackY{},
      mStackCount(0) {
  std::memset(mStackX, 0, sizeof(mStackX));
  std::memset(mStackY, 0, sizeof(mStackY));
}

void MazeDolphinSharks::Seed(unsigned char* pPassword, int pPasswordLength) {
  mRuntimeStats = RuntimeStats{};
  mRuntimeStats.mIsDolphin = 1U;
  InitializeSeedBuffer(pPassword, pPasswordLength, mCounter);
  mFastRand.SetSeedBuffer(mSeedBuffer, static_cast<int>(mResultBufferLength));
  mFastRand.Seed(pPassword, pPasswordLength);

  Build();
  Flush();
}

int MazeDolphinSharks::NextIndex(int pLimit) {
  if (pLimit <= 1) {
    return 0;
  }
  return static_cast<int>(mFastRand.GetInt() % static_cast<unsigned int>(pLimit));
}

void MazeDolphinSharks::FillStackAllCoords() {
  mStackCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      mStackX[mStackCount] = aX;
      mStackY[mStackCount] = aY;
      ++mStackCount;
    }
  }
}

void MazeDolphinSharks::ShuffleStack() {
  for (int aIndex = mStackCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(mStackX[aIndex], mStackX[aSwapIndex]);
    std::swap(mStackY[aIndex], mStackY[aSwapIndex]);
  }
}

void MazeDolphinSharks::SetInitialWalls() {
  ClearWalls();
  const int aWalls = (kGridSize / 3);
  for (int aIndex = 0; aIndex < aWalls && aIndex < mStackCount; ++aIndex) {
    const int aX = mStackX[aIndex];
    const int aY = mStackY[aIndex];
    if (IsEdge(aX, aY)) {
      continue;
    }
    SetWall(aX, aY, true);
  }
}

int MazeDolphinSharks::CollectOpenComponents(bool pMarkLabels,
                                             int* pComponentLabel,
                                             int* pComponentCount,
                                             bool* pTouchesEdge) {
  bool aVisited[kGridWidth][kGridHeight] = {};
  int aComponentCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (IsWall(aX, aY) || aVisited[aX][aY]) {
        continue;
      }

      bool aTouchesEdge = false;
      const int aComponentId = aComponentCount;
      ++aComponentCount;

      mStackCount = 0;
      mStackX[mStackCount] = aX;
      mStackY[mStackCount] = aY;
      ++mStackCount;
      aVisited[aX][aY] = true;

      while (mStackCount > 0) {
        --mStackCount;
        const int aCX = mStackX[mStackCount];
        const int aCY = mStackY[mStackCount];

        if (IsEdge(aCX, aCY)) {
          aTouchesEdge = true;
        }

        if (pMarkLabels && pComponentLabel != nullptr) {
          pComponentLabel[aCY * kGridWidth + aCX] = aComponentId;
        }

        const int aNX[4] = {aCX - 1, aCX + 1, aCX, aCX};
        const int aNY[4] = {aCY, aCY, aCY - 1, aCY + 1};
        for (int aDir = 0; aDir < 4; ++aDir) {
          if (!InBounds(aNX[aDir], aNY[aDir]) || IsWall(aNX[aDir], aNY[aDir]) || aVisited[aNX[aDir]][aNY[aDir]]) {
            continue;
          }
          aVisited[aNX[aDir]][aNY[aDir]] = true;
          mStackX[mStackCount] = aNX[aDir];
          mStackY[mStackCount] = aNY[aDir];
          ++mStackCount;
        }
      }

      if (pTouchesEdge != nullptr) {
        pTouchesEdge[aComponentId] = aTouchesEdge;
      }
    }
  }

  if (pComponentCount != nullptr) {
    *pComponentCount = aComponentCount;
  }
  return aComponentCount;
}

bool MazeDolphinSharks::OpenWallForEnclosedComponent(int pComponentId, const int* pComponentLabel) {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (pComponentLabel[aY * kGridWidth + aX] != pComponentId) {
        continue;
      }
      const int aNX[4] = {aX - 1, aX + 1, aX, aX};
      const int aNY[4] = {aY, aY, aY - 1, aY + 1};
      for (int aDir = 0; aDir < 4; ++aDir) {
        if (!InBounds(aNX[aDir], aNY[aDir]) || !IsWall(aNX[aDir], aNY[aDir])) {
          continue;
        }
        SetWall(aNX[aDir], aNY[aDir], false);
        return true;
      }
    }
  }
  return false;
}

bool MazeDolphinSharks::OpenWallToMergeComponents(const int* pComponentLabel, int pComponentCount) {
  for (int aY = 1; aY < kGridHeight - 1; ++aY) {
    for (int aX = 1; aX < kGridWidth - 1; ++aX) {
      if (!IsWall(aX, aY)) {
        continue;
      }
      int aLabelA = -1;
      int aLabelB = -1;
      const int aNX[4] = {aX - 1, aX + 1, aX, aX};
      const int aNY[4] = {aY, aY, aY - 1, aY + 1};
      for (int aDir = 0; aDir < 4; ++aDir) {
        const int aLabel = pComponentLabel[aNY[aDir] * kGridWidth + aNX[aDir]];
        if (aLabel < 0 || aLabel >= pComponentCount) {
          continue;
        }
        if (aLabelA < 0) {
          aLabelA = aLabel;
        } else if (aLabel != aLabelA) {
          aLabelB = aLabel;
          break;
        }
      }
      if (aLabelA >= 0 && aLabelB >= 0) {
        SetWall(aX, aY, false);
        return true;
      }
    }
  }

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (IsWall(aX, aY)) {
        SetWall(aX, aY, false);
        return true;
      }
    }
  }
  return false;
}

void MazeDolphinSharks::Build() {
  FillStackAllCoords();
  ShuffleStack();
  SetInitialWalls();

  static constexpr int kBuildCap = 20000;
  int aComponentLabel[kGridSize];
  bool aTouchesEdge[kGridSize];
  int aComponentCount = 0;

  for (int aIter = 0; aIter < kBuildCap; ++aIter) {
    std::memset(aComponentLabel, -1, sizeof(aComponentLabel));
    std::memset(aTouchesEdge, 0, sizeof(aTouchesEdge));
    (void)CollectOpenComponents(true, aComponentLabel, &aComponentCount, aTouchesEdge);

    bool aFoundIllegal = false;
    for (int aComp = 0; aComp < aComponentCount; ++aComp) {
      if (!aTouchesEdge[aComp]) {
        aFoundIllegal = true;
        (void)OpenWallForEnclosedComponent(aComp, aComponentLabel);
        break;
      }
    }
    if (aFoundIllegal) {
      continue;
    }

    if (aComponentCount <= 1) {
      break;
    }
    if (!OpenWallToMergeComponents(aComponentLabel, aComponentCount)) {
      break;
    }
  }
}

}  // namespace bread::maze
