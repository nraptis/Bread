#include "MazeSpecialEvents.hpp"

namespace peanutbutter::maze {

namespace {

using helpers::MazeRobot;
using helpers::MazeShark;

int AbsInt(int pValue) {
  return (pValue < 0) ? -pValue : pValue;
}

bool IsInRobotCollectBox(MazeRobot* const* pRobots, int pRobotCount, int pX, int pY) {
  for (int aRobotIndex = 0; aRobotIndex < pRobotCount; ++aRobotIndex) {
    const MazeRobot* aRobot = pRobots[aRobotIndex];
    if (aRobot == nullptr || aRobot->mDeadFlag) {
      continue;
    }
    if (AbsInt(aRobot->mX - pX) <= 1 && AbsInt(aRobot->mY - pY) <= 1) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool MazeSpecialEvents::RepaintOrFlushTile(Maze& pMaze, int pX, int pY) {
  if (!pMaze.InBounds(pX, pY) || pMaze.IsWall(pX, pY)) {
    return false;
  }

  const bool aHadByte = (pMaze.mIsByte[pX][pY] != 0);
  if (pMaze.RepaintFromSeed(pX, pY)) {
    ++pMaze.mRuntimeStats.mTilesPaintedValidScenario;
    return true;
  }
  if (aHadByte) {
    pMaze.Flush(pX, pY);
    return true;
  }
  return false;
}

void MazeSpecialEvents::StarBurst(Maze& pMaze,
                                  helpers::MazeRobot* const* pRobots,
                                  int pRobotCount) {
  ++pMaze.mRuntimeStats.mStarBurst;
  for (int aY = 0; aY < Maze::kGridHeight; ++aY) {
    for (int aX = 0; aX < Maze::kGridWidth; ++aX) {
      if (IsInRobotCollectBox(pRobots, pRobotCount, aX, aY)) {
        continue;
      }
      (void)RepaintOrFlushTile(pMaze, aX, aY);
    }
  }
}

void MazeSpecialEvents::ChaosStorm(Maze& pMaze,
                                   helpers::MazeShark* const* pSharks,
                                   int pSharkCount) {
  ++pMaze.mRuntimeStats.mChaosStorm;
  for (int aSharkIndex = 0; aSharkIndex < pSharkCount; ++aSharkIndex) {
    const MazeShark* aShark = pSharks[aSharkIndex];
    if (aShark == nullptr || aShark->mDeadFlag) {
      continue;
    }
    for (int aYOffset = -1; aYOffset <= 1; ++aYOffset) {
      for (int aXOffset = -1; aXOffset <= 1; ++aXOffset) {
        (void)RepaintOrFlushTile(pMaze, aShark->mX + aXOffset, aShark->mY + aYOffset);
      }
    }
  }
}

void MazeSpecialEvents::CometTrailsHorizontal(Maze& pMaze, const int* pRows, int pRowCount) {
  if (pRows == nullptr || pRowCount <= 0) {
    return;
  }

  ++pMaze.mRuntimeStats.mCometTrailsHorizontal;
  for (int aRowIndex = 0; aRowIndex < pRowCount; ++aRowIndex) {
    for (int aX = 0; aX < Maze::kGridWidth; ++aX) {
      (void)RepaintOrFlushTile(pMaze, aX, pRows[aRowIndex]);
    }
  }
}

void MazeSpecialEvents::CometTrailsVertical(Maze& pMaze, const int* pCols, int pColCount) {
  if (pCols == nullptr || pColCount <= 0) {
    return;
  }

  ++pMaze.mRuntimeStats.mCometTrailsVertical;
  for (int aColIndex = 0; aColIndex < pColCount; ++aColIndex) {
    for (int aY = 0; aY < Maze::kGridHeight; ++aY) {
      (void)RepaintOrFlushTile(pMaze, pCols[aColIndex], aY);
    }
  }
}

}  // namespace peanutbutter::maze
