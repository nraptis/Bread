#include "Maze.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "../fast_rand/FastRand.hpp"

namespace peanutbutter::maze {

Maze::Maze()
    : peanutbutter::rng::Shuffler(),
      MazeGrid(),
      mIsByte{},
      mByte{},
      mRuntimeStats{},
      mFlushAccountingMode(FlushAccountingMode::kRegular),
      mFlushX{},
      mFlushY{} {
  std::memset(mIsByte, 0, sizeof(mIsByte));
  std::memset(mByte, 0, sizeof(mByte));
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
  std::memset(mIsByte, 0, sizeof(mIsByte));
  std::memset(mByte, 0, sizeof(mByte));
}

void Maze::Repaint(int pX, int pY, unsigned char pValue) {
  if (!InBounds(pX, pY)) {
    return;
  }
  Flush(pX, pY);
  mByte[pX][pY] = pValue;
  mIsByte[pX][pY] = 1;
}

void Maze::Flush(int pX, int pY) {
  if (!InBounds(pX, pY)) {
    return;
  }
  if (mIsByte[pX][pY] == 0) {
    return;
  }
  mIsByte[pX][pY] = 0;
  EnqueueByte(mByte[pX][pY]);
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
  mByte[pX][pY] = pByte;
  mIsByte[pX][pY] = pIsByte ? 1 : 0;
}

void Maze::Flush() {
  FillAxisOrder(mFlushX, kGridWidth);
  bool aDidFlush = false;
  for (int aXI = 0; aXI < kGridWidth; ++aXI) {
    const int aX = mFlushX[aXI];
    FillAxisOrder(mFlushY, kGridHeight);
    for (int aYI = 0; aYI < kGridHeight; ++aYI) {
      const int aY = mFlushY[aYI];
      if (mIsByte[aX][aY] == 0) {
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

}  // namespace peanutbutter::maze
