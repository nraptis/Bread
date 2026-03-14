#include "src/maze/Maze.hpp"

#include <algorithm>
#include <climits>
#include <cstring>

#include "src/core/Bread.hpp"

namespace bread::maze {

Maze::Maze(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
           bread::rng::Counter* pCounter)
    : mWall{},
      mStackX{},
      mStackY{},
      mStackCount(0),
      mSafeX{},
      mSafeY{},
      mSafeByte{},
      mSafeCount(0),
      mRobotX(0),
      mRobotY(0),
      mCheeseX(kGridWidth - 1),
      mCheeseY(kGridHeight - 1),
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
      mNodeStateByIndex{},
      mPasswordExpander(pPasswordExpander),
      mCounter(pCounter),
      mBuffer{},
      mBufferLength(0),
      mGetCursor(0),
      mPutCursor(0) {
  std::memset(mWall, 0, sizeof(mWall));
  std::memset(mStackX, 0, sizeof(mStackX));
  std::memset(mStackY, 0, sizeof(mStackY));
  std::memset(mSafeX, 0, sizeof(mSafeX));
  std::memset(mSafeY, 0, sizeof(mSafeY));
  std::memset(mSafeByte, 0, sizeof(mSafeByte));
  std::memset(mPathX, 0, sizeof(mPathX));
  std::memset(mPathY, 0, sizeof(mPathY));
  std::memset(mParentX, 0xFF, sizeof(mParentX));
  std::memset(mParentY, 0xFF, sizeof(mParentY));
  std::memset(mGScore, 0x7F, sizeof(mGScore));
  std::memset(mFScore, 0x7F, sizeof(mFScore));
  std::memset(mOpenHeap, 0, sizeof(mOpenHeap));
  std::memset(mOpenHeapPosByIndex, 0xFF, sizeof(mOpenHeapPosByIndex));
  std::memset(mNodeStateByIndex, 0, sizeof(mNodeStateByIndex));
  std::memset(mBuffer, 0, sizeof(mBuffer));
}

void Maze::Seed(unsigned char* pPassword, int pPasswordLength) {
  if (pPassword == nullptr || pPasswordLength <= 0) {
    static unsigned char aFallback = 0U;
    pPassword = &aFallback;
    pPasswordLength = 1;
  }

  mBufferLength = PASSWORD_EXPANDED_SIZE;
  mGetCursor = 0;
  mPutCursor = 0;

  if (mPasswordExpander != nullptr) {
    mPasswordExpander->Expand(pPassword, pPasswordLength, mBuffer);
  } else {
    for (int aIndex = 0; aIndex < mBufferLength; ++aIndex) {
      mBuffer[aIndex] = pPassword[aIndex % pPasswordLength];
    }
  }

  if (mCounter != nullptr) {
    mCounter->Seed(mBuffer, mBufferLength);
  }

  Build();
  mRobotX = 0;
  mRobotY = 0;
  mCheeseX = kGridWidth - 1;
  mCheeseY = kGridHeight - 1;
  mWall[mRobotX][mRobotY] = 0;
  mWall[mCheeseX][mCheeseY] = 0;
  if (!FindPath(mRobotX, mRobotY, mCheeseX, mCheeseY)) {
    EnsureSimpleCornerPath();
    (void)FindPath(mRobotX, mRobotY, mCheeseX, mCheeseY);
  }
  EncodeMazeToBuffer();
}

void Maze::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }
  if (mBufferLength <= 0) {
    std::memset(pDestination, 0, static_cast<std::size_t>(pDestinationLength));
    return;
  }

  for (int aIndex = 0; aIndex < pDestinationLength; ++aIndex) {
    if (mGetCursor >= mBufferLength) {
      mGetCursor = 0;
    }
    pDestination[aIndex] = mBuffer[mGetCursor++];
  }
}

unsigned char Maze::Get() {
  unsigned char aByte = 0U;
  Get(&aByte, 1);
  return aByte;
}

int Maze::NextByte() {
  if (mCounter != nullptr) {
    return static_cast<int>(mCounter->Get());
  }
  if (mBufferLength <= 0) {
    return 0;
  }
  if (mGetCursor >= mBufferLength) {
    mGetCursor = 0;
  }
  return static_cast<int>(mBuffer[mGetCursor++]);
}

int Maze::NextIndex(int pLimit) {
  if (pLimit <= 1) {
    return 0;
  }
  return NextByte() % pLimit;
}

void Maze::FillStackAllCoords() {
  mStackCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      mStackX[mStackCount] = aX;
      mStackY[mStackCount] = aY;
      ++mStackCount;
    }
  }
}

void Maze::ShuffleStack() {
  for (int aIndex = mStackCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(mStackX[aIndex], mStackX[aSwapIndex]);
    std::swap(mStackY[aIndex], mStackY[aSwapIndex]);
  }
}

void Maze::SetInitialWalls() {
  std::memset(mWall, 0, sizeof(mWall));
  const int aWalls = (kGridSize >> 2);
  for (int aIndex = 0; aIndex < aWalls && aIndex < mStackCount; ++aIndex) {
    const int aX = mStackX[aIndex];
    const int aY = mStackY[aIndex];
    mWall[aX][aY] = 1;
  }
}

int Maze::CollectOpenComponents(bool pMarkLabels,
                                int* pComponentLabel,
                                int* pComponentCount,
                                bool* pTouchesEdge) {
  bool aVisited[kGridWidth][kGridHeight] = {};
  int aComponentCount = 0;
  mSafeCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mWall[aX][aY] != 0 || aVisited[aX][aY]) {
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

        if (aCX == 0 || aCY == 0 || aCX == kGridWidth - 1 || aCY == kGridHeight - 1) {
          aTouchesEdge = true;
        }

        if (pMarkLabels && pComponentLabel != nullptr) {
          pComponentLabel[aCY * kGridWidth + aCX] = aComponentId;
        }

        if (mSafeCount < kGridSize) {
          mSafeX[mSafeCount] = aCX;
          mSafeY[mSafeCount] = aCY;
          mSafeByte[mSafeCount] = static_cast<unsigned char>(NextByte() & 0xFF);
          ++mSafeCount;
        }

        const int aNX[4] = {aCX - 1, aCX + 1, aCX, aCX};
        const int aNY[4] = {aCY, aCY, aCY - 1, aCY + 1};
        for (int aDir = 0; aDir < 4; ++aDir) {
          if (aNX[aDir] < 0 || aNX[aDir] >= kGridWidth || aNY[aDir] < 0 || aNY[aDir] >= kGridHeight) {
            continue;
          }
          if (aVisited[aNX[aDir]][aNY[aDir]] || mWall[aNX[aDir]][aNY[aDir]] != 0) {
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

bool Maze::OpenWallForEnclosedComponent(int pComponentId, const int* pComponentLabel) {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (pComponentLabel[aY * kGridWidth + aX] != pComponentId) {
        continue;
      }
      const int aNX[4] = {aX - 1, aX + 1, aX, aX};
      const int aNY[4] = {aY, aY, aY - 1, aY + 1};
      for (int aDir = 0; aDir < 4; ++aDir) {
        if (aNX[aDir] < 0 || aNX[aDir] >= kGridWidth || aNY[aDir] < 0 || aNY[aDir] >= kGridHeight) {
          continue;
        }
        if (mWall[aNX[aDir]][aNY[aDir]] != 0) {
          mWall[aNX[aDir]][aNY[aDir]] = 0;
          return true;
        }
      }
    }
  }
  return false;
}

bool Maze::OpenWallToMergeComponents(const int* pComponentLabel, int pComponentCount) {
  for (int aY = 1; aY < kGridHeight - 1; ++aY) {
    for (int aX = 1; aX < kGridWidth - 1; ++aX) {
      if (mWall[aX][aY] == 0) {
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
        mWall[aX][aY] = 0;
        return true;
      }
    }
  }

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mWall[aX][aY] != 0) {
        mWall[aX][aY] = 0;
        return true;
      }
    }
  }
  return false;
}

void Maze::EnsureSimpleCornerPath() {
  // Guaranteed corridor: top row to right edge, then down right edge.
  for (int aX = 0; aX < kGridWidth; ++aX) {
    mWall[aX][0] = 0;
  }
  for (int aY = 0; aY < kGridHeight; ++aY) {
    mWall[kGridWidth - 1][aY] = 0;
  }
  mWall[0][0] = 0;
  mWall[kGridWidth - 1][kGridHeight - 1] = 0;
}

void Maze::Build() {
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
      if (!IsInClosedList(aNeighborIndex) && (mWall[aCurrentX - 1][aCurrentY] == 0) &&
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
      if (!IsInClosedList(aNeighborIndex) && (mWall[aCurrentX + 1][aCurrentY] == 0) &&
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
      if (!IsInClosedList(aNeighborIndex) && (mWall[aCurrentX][aCurrentY - 1] == 0) &&
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
      if (!IsInClosedList(aNeighborIndex) && (mWall[aCurrentX][aCurrentY + 1] == 0) &&
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
  return !InBounds(pX, pY) || (mWall[pX][pY] != 0);
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
  return InBounds(pX, pY) && (mWall[pX][pY] == 0);
}

int Maze::HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const {
  return AbsInt(pEndX - ToX(pIndex)) + AbsInt(pEndY - ToY(pIndex));
}

int Maze::ToIndex(int pX, int pY) const {
  return (pY << kGridShift) + pX;
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
  mOpenHeapPosByIndex[aIndexA] = pPosA;
  mOpenHeapPosByIndex[aIndexB] = pPosB;
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

void Maze::EncodeMazeToBuffer() {
  const int aMazeBytes = kGridSize;
  const int aWrite = std::min(mBufferLength, aMazeBytes);
  int aIndex = 0;
  for (int aY = 0; aY < kGridHeight && aIndex < aWrite; ++aY) {
    for (int aX = 0; aX < kGridWidth && aIndex < aWrite; ++aX) {
      mBuffer[aIndex++] = static_cast<unsigned char>(mWall[aX][aY] ? 1U : 0U);
    }
  }
}

}  // namespace bread::maze
