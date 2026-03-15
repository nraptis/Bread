#include "src/maze/MazeRobotCheese.hpp"

#include <algorithm>
#include <cstring>

namespace bread::maze {

MazeRobotCheese::MazeRobotCheese(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
                                 bread::rng::Counter* pCounter)
    : Maze(),
      mCounter(pCounter),
      mFastRand(pPasswordExpander, bread::fast_rand::FastRandWrapMode::kWrapXOR),
      mRobotX(0),
      mRobotY(0),
      mCheeseX(kGridWidth - 1),
      mCheeseY(kGridHeight - 1) {}

void MazeRobotCheese::Seed(unsigned char* pPassword, int pPasswordLength) {
  mRuntimeStats = RuntimeStats{};
  mRuntimeStats.mIsRobot = 1U;
  InitializeSeedBuffer(pPassword, pPasswordLength, mCounter);
  mFastRand.SetSeedBuffer(mSeedBuffer, static_cast<int>(mResultBufferLength));
  mFastRand.Seed(pPassword, pPasswordLength);

  Build();

  mRobotX = 0;
  mRobotY = 0;
  mCheeseX = kGridWidth - 1;
  mCheeseY = kGridHeight - 1;
  SetWall(mRobotX, mRobotY, false);
  SetWall(mCheeseX, mCheeseY, false);
  EnsureSingleConnectedOpenGroup();
  if (!FindPath(mRobotX, mRobotY, mCheeseX, mCheeseY)) {
    EnsureSimpleCornerPath();
    EnsureSingleConnectedOpenGroup();
  }
  (void)FindPath(mRobotX, mRobotY, mCheeseX, mCheeseY);
  FinalizeWalls();
  Flush();
}

int MazeRobotCheese::RobotX() const {
  return mRobotX;
}

int MazeRobotCheese::RobotY() const {
  return mRobotY;
}

int MazeRobotCheese::CheeseX() const {
  return mCheeseX;
}

int MazeRobotCheese::CheeseY() const {
  return mCheeseY;
}

int MazeRobotCheese::NextIndex(int pLimit) {
  if (pLimit <= 1) {
    return 0;
  }
  return static_cast<int>(mFastRand.GetInt() % static_cast<unsigned int>(pLimit));
}

void MazeRobotCheese::ShuffleStack() {
  for (int aIndex = mStackCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(mStackX[aIndex], mStackX[aSwapIndex]);
    std::swap(mStackY[aIndex], mStackY[aSwapIndex]);
  }
}

void MazeRobotCheese::SetInitialWalls() {
  ClearWalls();
  const int aWalls = (kGridSize >> 2);
  for (int aIndex = 0; aIndex < aWalls && aIndex < mStackCount; ++aIndex) {
    SetWall(mStackX[aIndex], mStackY[aIndex], true);
  }
}

void MazeRobotCheese::EnsureSimpleCornerPath() {
  for (int aX = 0; aX < kGridWidth; ++aX) {
    SetWall(aX, 0, false);
  }
  for (int aY = 0; aY < kGridHeight; ++aY) {
    SetWall(kGridWidth - 1, aY, false);
  }
  SetWall(0, 0, false);
  SetWall(kGridWidth - 1, kGridHeight - 1, false);
}

void MazeRobotCheese::Build() {
  FillStackAllCoords();
  ShuffleStack();
  SetInitialWalls();
  EnsureSingleConnectedOpenGroup();
}

}  // namespace bread::maze
