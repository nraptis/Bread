#include "MazeCharacters.hpp"

#include <cstring>

namespace peanutbutter::maze::helpers {

MazeCheese::MazeCheese() : mX(-1), mY(-1) {}

void MazeCheese::Reset() {
  mX = -1;
  mY = -1;
}

MazeRobot::MazeRobot()
    : mCheese(nullptr),
      mX(-1),
      mY(-1),
      mPathX{},
      mPathY{},
      mPathLength(0),
      mPathIndex(0),
      mPowerUpTick(0),
      mPowerUp(PowerUpType::kNone),
      mDeadFlag(false),
      mDidRecentlyRepath(false),
      mIsVictorious(false),
      mNeedsRepath(false),
      mPendingStepX{},
      mPendingStepY{},
      mPendingStepCount(0) {}

void MazeRobot::Reset() {
  mCheese = nullptr;
  mX = -1;
  mY = -1;
  ClearPath();
  mPowerUpTick = 0;
  mPowerUp = PowerUpType::kNone;
  mDeadFlag = false;
  mDidRecentlyRepath = false;
  mIsVictorious = false;
  mNeedsRepath = false;
  ResetTickPlan();
}

bool MazeRobot::AssignPathAndStartWalk(const int* pPathX, const int* pPathY, int pLength) {
  if (pLength < 0 || pLength > peanutbutter::maze::MazeGrid::kGridSize) {
    ClearPath();
    return false;
  }
  if (pLength > 0 && (pPathX == nullptr || pPathY == nullptr)) {
    ClearPath();
    return false;
  }

  ClearPath();
  for (int aIndex = 0; aIndex < pLength; ++aIndex) {
    mPathX[aIndex] = pPathX[aIndex];
    mPathY[aIndex] = pPathY[aIndex];
  }
  mPathLength = pLength;
  mPathIndex = (mPathLength > 0) ? 1 : 0;
  return true;
}

bool MazeRobot::Update(const peanutbutter::maze::MazeGrid& pMaze) {
  ResetTickPlan();
  if (mDeadFlag) {
    return true;
  }

  if (mPathLength <= 0 || mPathIndex < 0 || mPathIndex > mPathLength) {
    return false;
  }

  int aPathIndex = mPathIndex;
  if (aPathIndex <= 0 && mPathLength > 1) {
    aPathIndex = 1;
  }
  if (aPathIndex >= 0 && aPathIndex < mPathLength) {
    const int aNextX = mPathX[aPathIndex];
    const int aNextY = mPathY[aPathIndex];
    if (aNextX < 0 || aNextX >= MazeGrid::kGridWidth || aNextY < 0 || aNextY >= MazeGrid::kGridHeight ||
        pMaze.IsWall(aNextX, aNextY)) {
      ResetTickPlan();
      return false;
    }

    mPendingStepX[mPendingStepCount] = aNextX;
    mPendingStepY[mPendingStepCount] = aNextY;
    ++mPendingStepCount;
  }

  return true;
}

bool MazeRobot::ReadPendingStep(int pIndex, int* pOutX, int* pOutY) const {
  if (pOutX == nullptr || pOutY == nullptr || pIndex < 0 || pIndex >= mPendingStepCount) {
    return false;
  }

  *pOutX = mPendingStepX[pIndex];
  *pOutY = mPendingStepY[pIndex];
  return true;
}

void MazeRobot::ApplyPendingStep(int pIndex) {
  if (pIndex < 0 || pIndex >= mPendingStepCount) {
    return;
  }

  mX = mPendingStepX[pIndex];
  mY = mPendingStepY[pIndex];
  ++mPathIndex;
}

void MazeRobot::FinishUpdate() {
  if (mPowerUpTick > 0) {
    --mPowerUpTick;
    if (mPowerUpTick <= 0) {
      mPowerUpTick = 0;
      mPowerUp = PowerUpType::kNone;
    }
  }
  mPendingStepCount = 0;
}

void MazeRobot::Die() {
  mDeadFlag = true;
  mDidRecentlyRepath = false;
  mIsVictorious = false;
  mNeedsRepath = false;
  mCheese = nullptr;
  ClearPath();
  mPowerUp = PowerUpType::kNone;
  mPowerUpTick = 0;
  ResetTickPlan();
}

void MazeRobot::ClearPath() {
  std::memset(mPathX, 0, sizeof(mPathX));
  std::memset(mPathY, 0, sizeof(mPathY));
  mPathLength = 0;
  mPathIndex = 0;
}

void MazeRobot::ResetTickPlan() {
  std::memset(mPendingStepX, 0, sizeof(mPendingStepX));
  std::memset(mPendingStepY, 0, sizeof(mPendingStepY));
  mPendingStepCount = 0;
}

MazeShark::MazeShark() : mX(-1), mY(-1), mDeadFlag(false), mDidRecentlyRevive(false) {}

void MazeShark::Reset() {
  mX = -1;
  mY = -1;
  mDeadFlag = false;
  mDidRecentlyRevive = false;
}

MazeDolphin::MazeDolphin() : mX(-1), mY(-1), mDeadFlag(false), mDidRecentlyRevive(false) {}

void MazeDolphin::Reset() {
  mX = -1;
  mY = -1;
  mDeadFlag = false;
  mDidRecentlyRevive = false;
}

}  // namespace peanutbutter::maze::helpers
