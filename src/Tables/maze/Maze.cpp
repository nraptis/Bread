#include "Maze.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "../fast_rand/FastRand.hpp"

namespace peanutbutter::maze {

Maze::Maze()
    : peanutbutter::rng::Shuffler(),
      MazeGrid(),
      mByte{},
      mRobotStorage{},
      mCheeseStorage{},
      mSharkStorage{},
      mDolphinStorage{},
      mRobotList{},
      mRobotListCount(0),
      mCheeseList{},
      mCheeseListCount(0),
      mSharkList{},
      mSharkListCount(0),
      mDolphinList{},
      mDolphinListCount(0),
      mRuntimeStats{},
      mFlushAccountingMode(FlushAccountingMode::kRegular),
      mFlushX{},
      mFlushY{} {
  ClearByteCells();
  std::memset(mFlushX, 0, sizeof(mFlushX));
  std::memset(mFlushY, 0, sizeof(mFlushY));
}

Maze::RuntimeStats Maze::GetRuntimeStats() const {
  return mRuntimeStats;
}

void Maze::InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength) {
  if (pPasswordLength != kSeedBufferCapacity) {
    std::abort();
  }
  peanutbutter::rng::Shuffler::InitializeSeedBuffer(pPassword, pPasswordLength);
  ClearByteCells();
}

void Maze::ShuffleSeedBuffer(peanutbutter::fast_rand::FastRand* pFastRand) {
  if (pFastRand == nullptr || mResultBufferLength <= 1U) {
    return;
  }

  for (unsigned int aIndex = mResultBufferLength - 1U; aIndex > 0U; --aIndex) {
    const unsigned int aSwapIndex = pFastRand->GetInt() % (aIndex + 1U);
    const unsigned char aTemp = mSeedBuffer[aIndex];
    mSeedBuffer[aIndex] = mSeedBuffer[aSwapIndex];
    mSeedBuffer[aSwapIndex] = aTemp;
  }
}

void Maze::Reset() {
  Flush();
  ResetGrid();
}

bool Maze::PaintWalkableFromSeed() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsWall[aX][aY] != 0) {
        continue;
      }
      if (!RepaintFromSeed(aX, aY)) {
        return false;
      }
    }
  }
  return true;
}

bool Maze::RepaintFromSeed(int pX, int pY) {
  if (!SeedCanDequeue()) {
    return false;
  }
  Repaint(pX, pY, SeedDequeue());
  return true;
}

void Maze::ApocalypseScenario() {
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    return;
  }

  for (unsigned int aIndex = 0U; aIndex < mResultBufferLength; ++aIndex) {
    mResultBuffer[aIndex] = mSeedBuffer[aIndex % static_cast<unsigned int>(kSeedBufferCapacity)];
  }

  unsigned int aLeft = 0U;
  unsigned int aRight = mResultBufferLength - 1U;
  while (aLeft < aRight) {
    const unsigned char aTemp = mResultBuffer[aLeft];
    mResultBuffer[aLeft] = mResultBuffer[aRight];
    mResultBuffer[aRight] = aTemp;
    ++aLeft;
    --aRight;
  }

  mSeedReadIndex = mResultBufferLength;
  mResultBufferWriteIndex = 0U;
  mSeedBytesRemaining = 0U;
  mResultBufferWriteProgress = mResultBufferLength;
}

void Maze::SetFlushAccountingMode(FlushAccountingMode pMode) {
  mFlushAccountingMode = pMode;
}

void Maze::ClearByteCells() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      mByte[aX][aY] = -1;
    }
  }
}

void Maze::Repaint(int pX, int pY, unsigned char pValue) {
  if (!InBounds(pX, pY)) {
    return;
  }
  Flush(pX, pY);
  mByte[pX][pY] = static_cast<int>(pValue);
}

void Maze::Flush(int pX, int pY) {
  if (!InBounds(pX, pY)) {
    return;
  }
  if (mByte[pX][pY] < 0) {
    return;
  }
  EnqueueByte(static_cast<unsigned char>(mByte[pX][pY]));
  mByte[pX][pY] = -1;
  if (mFlushAccountingMode == FlushAccountingMode::kStalled) {
    ++mRuntimeStats.mTilesFlushedStalled;
  } else {
    ++mRuntimeStats.mTilesFlushedRegular;
  }
}

void Maze::SetByteCell(int pX, int pY, unsigned char pByte, bool pIsByte) {
  if (!InBounds(pX, pY)) {
    return;
  }
  mByte[pX][pY] = pIsByte ? static_cast<int>(pByte) : -1;
}

void Maze::Flush() {
  FillAxisOrder(mFlushX, kGridWidth);
  bool aDidFlush = false;
  for (int aXI = 0; aXI < kGridWidth; ++aXI) {
    const int aX = mFlushX[aXI];
    FillAxisOrder(mFlushY, kGridHeight);
    for (int aYI = 0; aYI < kGridHeight; ++aYI) {
      const int aY = mFlushY[aYI];
      if (mByte[aX][aY] < 0) {
        continue;
      }
      Flush(aX, aY);
      aDidFlush = true;
    }
  }

  if (aDidFlush) {
    ++mRuntimeStats.mFlush;
  } else {
    ++mRuntimeStats.mEmptyFlush;
  }
}

void Maze::ResetCharacterLists(int pRobotCount, int pCheeseCount, int pSharkCount, int pDolphinCount) {
  std::memset(mRobotList, 0, sizeof(mRobotList));
  std::memset(mCheeseList, 0, sizeof(mCheeseList));
  std::memset(mSharkList, 0, sizeof(mSharkList));
  std::memset(mDolphinList, 0, sizeof(mDolphinList));
  mRobotListCount = 0;
  mCheeseListCount = 0;
  mSharkListCount = 0;
  mDolphinListCount = 0;

  const int aRobotCount =
      (pRobotCount <= 0) ? 0 : ((pRobotCount < helpers::kMaxRobots) ? pRobotCount : helpers::kMaxRobots);
  const int aCheeseCount =
      (pCheeseCount <= 0) ? 0 : ((pCheeseCount < helpers::kMaxCheeses) ? pCheeseCount : helpers::kMaxCheeses);
  const int aSharkCount =
      (pSharkCount <= 0) ? 0 : ((pSharkCount < helpers::kMaxSharks) ? pSharkCount : helpers::kMaxSharks);
  const int aDolphinCount =
      (pDolphinCount <= 0) ? 0 : ((pDolphinCount < helpers::kMaxDolphins) ? pDolphinCount : helpers::kMaxDolphins);

  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    mRobotList[mRobotListCount] = &mRobotStorage[aRobotIndex];
    ++mRobotListCount;
  }
  for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
    mCheeseList[mCheeseListCount] = &mCheeseStorage[aCheeseIndex];
    ++mCheeseListCount;
  }
  for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
    mSharkList[mSharkListCount] = &mSharkStorage[aSharkIndex];
    ++mSharkListCount;
  }
  for (int aDolphinIndex = 0; aDolphinIndex < aDolphinCount; ++aDolphinIndex) {
    mDolphinList[mDolphinListCount] = &mDolphinStorage[aDolphinIndex];
    ++mDolphinListCount;
  }
}

}  // namespace peanutbutter::maze
