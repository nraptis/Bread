#include "src/maze/MazeDolphinSharks.hpp"

#include <cstring>
#include <utility>

namespace bread::maze {

MazeDolphinSharks::MazeDolphinSharks(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
                                     bread::rng::Counter* pCounter)
    : Maze(),
      mCounter(pCounter),
      mFastRand(pPasswordExpander, bread::fast_rand::FastRandWrapMode::kWrapXOR) {}

void MazeDolphinSharks::Seed(unsigned char* pPassword, int pPasswordLength) {
  mRuntimeStats = RuntimeStats{};
  mRuntimeStats.mIsDolphin = 1U;
  InitializeSeedBuffer(pPassword, pPasswordLength, mCounter);
  mFastRand.SetSeedBuffer(mSeedBuffer, static_cast<int>(mResultBufferLength));
  mFastRand.Seed(pPassword, pPasswordLength);

  Build();
  FinalizeWalls();
  Flush();
}

int MazeDolphinSharks::NextIndex(int pLimit) {
  if (pLimit <= 1) {
    return 0;
  }
  return static_cast<int>(mFastRand.GetInt() % static_cast<unsigned int>(pLimit));
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

void MazeDolphinSharks::Build() {
  FillStackAllCoords();
  ShuffleStack();
  SetInitialWalls();
  EnsureSingleConnectedOpenGroup();
}

}  // namespace bread::maze
