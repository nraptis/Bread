#include "src/maze/Maze.hpp"

#include <algorithm>
#include <climits>
#include <cstring>

#include "src/fast_rand/FastRand.hpp"
#include "src/rng/Counter.hpp"

namespace bread::maze {

Maze::Maze()
    : mIsWall{},
      mIsByte{},
      mByte{},
      mSeedBuffer{},
      mResultBuffer(nullptr),
      mResultBufferReadIndex(0U),
      mResultBufferWriteIndex(0U),
      mResultBufferLength(0U),
      mSeedBytesRemaining(0U),
      mResultBufferWriteProgress(0U),
      mRuntimeStats{},
      mPathX{},
      mPathY{},
      mPathLength(0),
      mParentX{},
      mParentY{},
      mGScore{},
      mFScore{},
      mOpenHeap{},
      mOpenHeapCount(0),
      mOpenHeapPosByIndex{},
      mNodeStateByIndex{} {
  std::memset(mIsWall, 0, sizeof(mIsWall));
  std::memset(mIsByte, 0, sizeof(mIsByte));
  std::memset(mByte, 0, sizeof(mByte));
  std::memset(mSeedBuffer, 0, sizeof(mSeedBuffer));
  std::memset(mPathX, 0, sizeof(mPathX));
  std::memset(mPathY, 0, sizeof(mPathY));
  std::memset(mParentX, 0xFF, sizeof(mParentX));
  std::memset(mParentY, 0xFF, sizeof(mParentY));
  std::memset(mGScore, 0x7F, sizeof(mGScore));
  std::memset(mFScore, 0x7F, sizeof(mFScore));
  std::memset(mOpenHeap, 0, sizeof(mOpenHeap));
  std::memset(mOpenHeapPosByIndex, 0xFF, sizeof(mOpenHeapPosByIndex));
  std::memset(mNodeStateByIndex, 0, sizeof(mNodeStateByIndex));
}

unsigned char Maze::Get() {
  unsigned char aByte = 0U;
  Get(&aByte, 1);
  return aByte;
}

void Maze::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    std::memset(pDestination, 0, static_cast<std::size_t>(pDestinationLength));
    return;
  }

  int aOffset = 0;
  while (aOffset < pDestinationLength) {
    const unsigned int aToEnd = mResultBufferLength - mResultBufferReadIndex;
    const int aTake = std::min(pDestinationLength - aOffset, static_cast<int>(aToEnd));
    std::memcpy(pDestination + aOffset,
                mResultBuffer + static_cast<std::ptrdiff_t>(mResultBufferReadIndex),
                static_cast<std::size_t>(aTake));
    mResultBufferReadIndex = (mResultBufferReadIndex + static_cast<unsigned int>(aTake)) % mResultBufferLength;
    aOffset += aTake;
  }
}

Maze::RuntimeStats Maze::GetRuntimeStats() const {
  return mRuntimeStats;
}

void Maze::InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength, bread::rng::Counter* pCounter) {
  if (pPassword == nullptr || pPasswordLength <= 0) {
    static unsigned char aFallback = 0U;
    pPassword = &aFallback;
    pPasswordLength = 1;
  }

  mResultBuffer = mSeedBuffer;
  mResultBufferLength = static_cast<unsigned int>(std::min(pPasswordLength, kSeedBufferCapacity));
  mResultBufferReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mSeedBytesRemaining = mResultBufferLength;
  mResultBufferWriteProgress = 0U;
  ClearByteCells();

  if (pCounter != nullptr) {
    pCounter->Seed(pPassword, pPasswordLength);
    pCounter->Get(mSeedBuffer, static_cast<int>(mResultBufferLength));
  } else {
    for (unsigned int aIndex = 0U; aIndex < mResultBufferLength; ++aIndex) {
      mSeedBuffer[aIndex] =
          pPassword[static_cast<std::size_t>(aIndex) % static_cast<std::size_t>(pPasswordLength)];
    }
  }
}

bool Maze::SeedCanDequeue() const {
  return mResultBuffer != nullptr && mResultBufferLength > 0U && mSeedBytesRemaining > 0U;
}

unsigned char Maze::SeedDequeue() {
  if (!SeedCanDequeue()) {
    return 0U;
  }
  const unsigned char aByte = mResultBuffer[mResultBufferReadIndex];
  mResultBufferReadIndex = (mResultBufferReadIndex + 1U) % mResultBufferLength;
  --mSeedBytesRemaining;
  return aByte;
}

void Maze::EnqueueByte(unsigned char pByte) {
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    return;
  }
  mResultBuffer[mResultBufferWriteIndex] = pByte;
  mResultBufferWriteIndex = (mResultBufferWriteIndex + 1U) % mResultBufferLength;
  ++mResultBufferWriteProgress;
}

void Maze::ShuffleSeedBuffer(bread::fast_rand::FastRand* pFastRand) {
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

bool Maze::FindPath(int pStartX, int pStartY, int pEndX, int pEndY) {
  mPathLength = 0;
  mOpenHeapCount = 0;
  std::memset(mOpenHeapPosByIndex, 0xFF, sizeof(mOpenHeapPosByIndex));
  std::memset(mNodeStateByIndex, 0, sizeof(mNodeStateByIndex));

  if (!InBounds(pStartX, pStartY) || !InBounds(pEndX, pEndY)) {
    return false;
  }
  if (!IsWalkable(pStartX, pStartY) || !IsWalkable(pEndX, pEndY)) {
    return false;
  }

  for (int aIndex = 0; aIndex < kGridSize; ++aIndex) {
    mParentX[aIndex] = -1;
    mParentY[aIndex] = -1;
    mGScore[aIndex] = INT_MAX;
    mFScore[aIndex] = INT_MAX;
  }

  const int aStartIndex = ToIndex(pStartX, pStartY);
  const int aEndIndex = ToIndex(pEndX, pEndY);
  mGScore[aStartIndex] = 0;
  mFScore[aStartIndex] = HeuristicCostByIndex(aStartIndex, pEndX, pEndY);
  PushOpenNode(aStartIndex);

  while (mOpenHeapCount > 0) {
    const int aCurrentIndex = RemoveOpenMin();
    if (aCurrentIndex < 0) {
      break;
    }
    if (IsInClosedList(aCurrentIndex)) {
      continue;
    }

    mNodeStateByIndex[aCurrentIndex] = 2U;

    if (aCurrentIndex == aEndIndex) {
      ReconstructPath(aEndIndex);
      return (mPathLength > 0);
    }

    const int aCurrentX = ToX(aCurrentIndex);
    const int aCurrentY = ToY(aCurrentIndex);
    const int aBaseG = mGScore[aCurrentIndex] + 1;

    if (aCurrentX > 0) {
      const int aNeighborIndex = aCurrentIndex - 1;
      if (!IsInClosedList(aNeighborIndex) && (mIsWall[aCurrentX - 1][aCurrentY] == 0) &&
          (aBaseG < mGScore[aNeighborIndex])) {
        mParentX[aNeighborIndex] = aCurrentX;
        mParentY[aNeighborIndex] = aCurrentY;
        mGScore[aNeighborIndex] = aBaseG;
        mFScore[aNeighborIndex] = aBaseG + HeuristicCostByIndex(aNeighborIndex, pEndX, pEndY);
        if (IsInOpenList(aNeighborIndex)) {
          UpdateOpenNodePriority(aNeighborIndex);
        } else {
          PushOpenNode(aNeighborIndex);
        }
      }
    }
    if (aCurrentX < (kGridWidth - 1)) {
      const int aNeighborIndex = aCurrentIndex + 1;
      if (!IsInClosedList(aNeighborIndex) && (mIsWall[aCurrentX + 1][aCurrentY] == 0) &&
          (aBaseG < mGScore[aNeighborIndex])) {
        mParentX[aNeighborIndex] = aCurrentX;
        mParentY[aNeighborIndex] = aCurrentY;
        mGScore[aNeighborIndex] = aBaseG;
        mFScore[aNeighborIndex] = aBaseG + HeuristicCostByIndex(aNeighborIndex, pEndX, pEndY);
        if (IsInOpenList(aNeighborIndex)) {
          UpdateOpenNodePriority(aNeighborIndex);
        } else {
          PushOpenNode(aNeighborIndex);
        }
      }
    }
    if (aCurrentY > 0) {
      const int aNeighborIndex = aCurrentIndex - kGridWidth;
      if (!IsInClosedList(aNeighborIndex) && (mIsWall[aCurrentX][aCurrentY - 1] == 0) &&
          (aBaseG < mGScore[aNeighborIndex])) {
        mParentX[aNeighborIndex] = aCurrentX;
        mParentY[aNeighborIndex] = aCurrentY;
        mGScore[aNeighborIndex] = aBaseG;
        mFScore[aNeighborIndex] = aBaseG + HeuristicCostByIndex(aNeighborIndex, pEndX, pEndY);
        if (IsInOpenList(aNeighborIndex)) {
          UpdateOpenNodePriority(aNeighborIndex);
        } else {
          PushOpenNode(aNeighborIndex);
        }
      }
    }
    if (aCurrentY < (kGridHeight - 1)) {
      const int aNeighborIndex = aCurrentIndex + kGridWidth;
      if (!IsInClosedList(aNeighborIndex) && (mIsWall[aCurrentX][aCurrentY + 1] == 0) &&
          (aBaseG < mGScore[aNeighborIndex])) {
        mParentX[aNeighborIndex] = aCurrentX;
        mParentY[aNeighborIndex] = aCurrentY;
        mGScore[aNeighborIndex] = aBaseG;
        mFScore[aNeighborIndex] = aBaseG + HeuristicCostByIndex(aNeighborIndex, pEndX, pEndY);
        if (IsInOpenList(aNeighborIndex)) {
          UpdateOpenNodePriority(aNeighborIndex);
        } else {
          PushOpenNode(aNeighborIndex);
        }
      }
    }
  }

  return false;
}

int Maze::PathLength() const {
  return mPathLength;
}

bool Maze::PathNode(int pIndex, int* pOutX, int* pOutY) const {
  if (pOutX == nullptr || pOutY == nullptr) {
    return false;
  }
  if (pIndex < 0 || pIndex >= mPathLength) {
    return false;
  }
  *pOutX = mPathX[pIndex];
  *pOutY = mPathY[pIndex];
  return true;
}

bool Maze::IsWall(int pX, int pY) const {
  return !InBounds(pX, pY) || (mIsWall[pX][pY] != 0);
}

bool Maze::IsEdge(int pX, int pY) const {
  if (!InBounds(pX, pY)) {
    return false;
  }
  return (pX == 0 || pY == 0 || pX == kGridWidth - 1 || pY == kGridHeight - 1);
}

bool Maze::IsConnected_Slow(int pX1, int pY1, int pX2, int pY2) const {
  if (!InBounds(pX1, pY1) || !InBounds(pX2, pY2)) {
    return false;
  }
  if (IsWall(pX1, pY1) || IsWall(pX2, pY2)) {
    return false;
  }
  if (pX1 == pX2 && pY1 == pY2) {
    return true;
  }

  bool aVisited[kGridWidth][kGridHeight] = {};
  int aStackX[kGridSize];
  int aStackY[kGridSize];
  int aStackCount = 0;

  aVisited[pX1][pY1] = true;
  aStackX[aStackCount] = pX1;
  aStackY[aStackCount] = pY1;
  ++aStackCount;

  while (aStackCount > 0) {
    --aStackCount;
    const int aX = aStackX[aStackCount];
    const int aY = aStackY[aStackCount];

    if (aX == pX2 && aY == pY2) {
      return true;
    }

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};
    for (int aDir = 0; aDir < 4; ++aDir) {
      if (!InBounds(aNX[aDir], aNY[aDir]) || IsWall(aNX[aDir], aNY[aDir]) || aVisited[aNX[aDir]][aNY[aDir]]) {
        continue;
      }
      aVisited[aNX[aDir]][aNY[aDir]] = true;
      aStackX[aStackCount] = aNX[aDir];
      aStackY[aStackCount] = aNY[aDir];
      ++aStackCount;
    }
  }

  return false;
}

bool Maze::IsConnectedToEdge(int pX, int pY) const {
  if (!InBounds(pX, pY) || IsWall(pX, pY)) {
    return false;
  }
  if (IsEdge(pX, pY)) {
    return true;
  }

  bool aVisited[kGridWidth][kGridHeight] = {};
  int aStackX[kGridSize];
  int aStackY[kGridSize];
  int aStackCount = 0;

  aVisited[pX][pY] = true;
  aStackX[aStackCount] = pX;
  aStackY[aStackCount] = pY;
  ++aStackCount;

  while (aStackCount > 0) {
    --aStackCount;
    const int aX = aStackX[aStackCount];
    const int aY = aStackY[aStackCount];

    if (IsEdge(aX, aY)) {
      return true;
    }

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};
    for (int aDir = 0; aDir < 4; ++aDir) {
      if (!InBounds(aNX[aDir], aNY[aDir]) || IsWall(aNX[aDir], aNY[aDir]) || aVisited[aNX[aDir]][aNY[aDir]]) {
        continue;
      }
      aVisited[aNX[aDir]][aNY[aDir]] = true;
      aStackX[aStackCount] = aNX[aDir];
      aStackY[aStackCount] = aNY[aDir];
      ++aStackCount;
    }
  }

  return false;
}

int Maze::AbsInt(int pValue) {
  return (pValue < 0) ? -pValue : pValue;
}

int Maze::ToX(int pIndex) const {
  return pIndex & kGridMask;
}

int Maze::ToY(int pIndex) const {
  return pIndex >> kGridShift;
}

bool Maze::InBounds(int pX, int pY) const {
  return (pX >= 0 && pX < kGridWidth && pY >= 0 && pY < kGridHeight);
}

bool Maze::IsWalkable(int pX, int pY) const {
  return InBounds(pX, pY) && (mIsWall[pX][pY] == 0);
}

int Maze::HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const {
  return AbsInt(pEndX - ToX(pIndex)) + AbsInt(pEndY - ToY(pIndex));
}

int Maze::ToIndex(int pX, int pY) const {
  return (pY << kGridShift) + pX;
}

void Maze::ClearWalls() {
  std::memset(mIsWall, 0, sizeof(mIsWall));
}

void Maze::ClearByteCells() {
  std::memset(mIsByte, 0, sizeof(mIsByte));
  std::memset(mByte, 0, sizeof(mByte));
}

void Maze::SetWall(int pX, int pY, bool pIsWall) {
  if (!InBounds(pX, pY)) {
    return;
  }
  mIsWall[pX][pY] = pIsWall ? 1 : 0;
}

void Maze::SetByteCell(int pX, int pY, unsigned char pByte, bool pIsByte) {
  if (!InBounds(pX, pY)) {
    return;
  }
  mByte[pX][pY] = pByte;
  mIsByte[pX][pY] = pIsByte ? 1 : 0;
}

void Maze::Flush() {
  bool aDidFlush = false;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsByte[aX][aY] == 0) {
        continue;
      }
      mIsByte[aX][aY] = 0;
      EnqueueByte(mByte[aX][aY]);
      aDidFlush = true;
    }
  }

  if (aDidFlush) {
    ++mRuntimeStats.mFlush;
  } else {
    ++mRuntimeStats.mEmptyFlush;
  }
}

bool Maze::OpenNodeLess(int pPosA, int pPosB) const {
  const int aIndexA = mOpenHeap[pPosA];
  const int aIndexB = mOpenHeap[pPosB];
  const int aFA = mFScore[aIndexA];
  const int aFB = mFScore[aIndexB];
  if (aFA != aFB) {
    return aFA < aFB;
  }
  return mGScore[aIndexA] < mGScore[aIndexB];
}

void Maze::SwapOpenNodes(int pPosA, int pPosB) {
  std::swap(mOpenHeap[pPosA], mOpenHeap[pPosB]);
  const int aIndexA = mOpenHeap[pPosA];
  const int aIndexB = mOpenHeap[pPosB];
  mOpenHeapPosByIndex[aIndexA] = static_cast<short>(pPosA);
  mOpenHeapPosByIndex[aIndexB] = static_cast<short>(pPosB);
}

void Maze::HeapifyUp(int pPos) {
  int aPos = pPos;
  while (aPos > 0) {
    const int aParent = (aPos - 1) >> 1;
    if (!OpenNodeLess(aPos, aParent)) {
      break;
    }
    SwapOpenNodes(aPos, aParent);
    aPos = aParent;
  }
}

void Maze::HeapifyDown(int pPos) {
  int aPos = pPos;
  for (;;) {
    const int aLeft = (aPos << 1) + 1;
    const int aRight = aLeft + 1;
    int aBest = aPos;

    if (aLeft < mOpenHeapCount && OpenNodeLess(aLeft, aBest)) {
      aBest = aLeft;
    }
    if (aRight < mOpenHeapCount && OpenNodeLess(aRight, aBest)) {
      aBest = aRight;
    }
    if (aBest == aPos) {
      break;
    }
    SwapOpenNodes(aPos, aBest);
    aPos = aBest;
  }
}

void Maze::PushOpenNode(int pIndex) {
  if (mOpenHeapCount >= kGridSize) {
    return;
  }
  if (mNodeStateByIndex[pIndex] != 0U) {
    return;
  }

  const int aPos = mOpenHeapCount;
  mOpenHeap[aPos] = pIndex;
  mOpenHeapPosByIndex[pIndex] = static_cast<short>(aPos);
  mNodeStateByIndex[pIndex] = 1U;
  ++mOpenHeapCount;
  HeapifyUp(aPos);
}

void Maze::UpdateOpenNodePriority(int pIndex) {
  if (mNodeStateByIndex[pIndex] != 1U) {
    return;
  }
  const int aPos = mOpenHeapPosByIndex[pIndex];
  if (aPos < 0 || aPos >= mOpenHeapCount) {
    return;
  }
  HeapifyUp(aPos);
}

int Maze::RemoveOpenMin() {
  if (mOpenHeapCount <= 0) {
    return -1;
  }

  const int aRootIndex = mOpenHeap[0];
  mNodeStateByIndex[aRootIndex] = 0U;
  mOpenHeapPosByIndex[aRootIndex] = -1;

  const int aLast = mOpenHeapCount - 1;
  if (aLast > 0) {
    mOpenHeap[0] = mOpenHeap[aLast];
    const int aMovedIndex = mOpenHeap[0];
    mOpenHeapPosByIndex[aMovedIndex] = 0;
  }
  --mOpenHeapCount;
  if (mOpenHeapCount > 0) {
    HeapifyDown(0);
  }
  return aRootIndex;
}

bool Maze::IsInClosedList(int pIndex) const {
  return mNodeStateByIndex[pIndex] == 2U;
}

bool Maze::IsInOpenList(int pIndex) const {
  return mNodeStateByIndex[pIndex] == 1U;
}

void Maze::ReconstructPath(int pEndIndex) {
  int aReverseX[kGridSize];
  int aReverseY[kGridSize];
  int aReverseCount = 0;

  int aX = ToX(pEndIndex);
  int aY = ToY(pEndIndex);
  while (aReverseCount < kGridSize && aX >= 0 && aY >= 0) {
    aReverseX[aReverseCount] = aX;
    aReverseY[aReverseCount] = aY;
    ++aReverseCount;
    const int aIndex = ToIndex(aX, aY);
    const int aParentNodeX = mParentX[aIndex];
    const int aParentNodeY = mParentY[aIndex];
    if (aParentNodeX < 0 || aParentNodeY < 0) {
      break;
    }
    aX = aParentNodeX;
    aY = aParentNodeY;
  }

  mPathLength = 0;
  for (int aIndex = aReverseCount - 1; aIndex >= 0; --aIndex) {
    if (mPathLength >= kGridSize) {
      break;
    }
    mPathX[mPathLength] = aReverseX[aIndex];
    mPathY[mPathLength] = aReverseY[aIndex];
    ++mPathLength;
  }
}

}  // namespace bread::maze
