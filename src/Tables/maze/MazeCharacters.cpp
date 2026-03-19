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
      mInvincibleTick(0),
      mMagnetTick(0),
      mTeleportTick(0),
      mInvincibleEnabled(false),
      mMagnetEnabled(false),
      mTeleportEnabled(false),
      mDeadFlag(false),
      mDidRecentlyRepath(false),
      mIsVictorious(false),
      mNeedsRepath(false) {}

void MazeRobot::Reset() {
  mCheese = nullptr;
  mX = -1;
  mY = -1;
  ClearPath();
  mInvincibleTick = 0;
  mMagnetTick = 0;
  mTeleportTick = 0;
  mInvincibleEnabled = false;
  mMagnetEnabled = false;
  mTeleportEnabled = false;
  mDeadFlag = false;
  mDidRecentlyRepath = false;
  mIsVictorious = false;
  mNeedsRepath = false;
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

void MazeRobot::Update() {
  if (mInvincibleTick > 0) {
    --mInvincibleTick;
    if (mInvincibleTick <= 0) {
      mInvincibleTick = 0;
      mInvincibleEnabled = false;
    }
  }
  if (mMagnetTick > 0) {
    --mMagnetTick;
    if (mMagnetTick <= 0) {
      mMagnetTick = 0;
      mMagnetEnabled = false;
    }
  }
  if (mTeleportTick > 0) {
    --mTeleportTick;
    if (mTeleportTick <= 0) {
      mTeleportTick = 0;
      mTeleportEnabled = false;
    }
  }
}

void MazeRobot::Die() {
  mDeadFlag = true;
  mDidRecentlyRepath = false;
  mIsVictorious = false;
  mNeedsRepath = false;
  mCheese = nullptr;
  ClearPath();
  mInvincibleTick = 0;
  mMagnetTick = 0;
  mTeleportTick = 0;
  mInvincibleEnabled = false;
  mMagnetEnabled = false;
  mTeleportEnabled = false;
}

void MazeRobot::ClearPath() {
  std::memset(mPathX, 0, sizeof(mPathX));
  std::memset(mPathY, 0, sizeof(mPathY));
  mPathLength = 0;
  mPathIndex = 0;
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
