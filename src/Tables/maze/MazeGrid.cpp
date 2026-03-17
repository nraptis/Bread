#include "MazeGrid.hpp"

#include <algorithm>
#include <climits>
#include <cstring>

namespace peanutbutter::maze {

MazeGrid::MazeGrid()
    : mIsWall{},
      mStackX{},
      mStackY{},
      mStackCount(0),
      mEdgeX{},
      mEdgeY{},
      mEdgeCount(0),
      mIsVisited{},
      mIsMarked{},
      mIsEdgeConnected{},
      mGroup{},
      mGroupIsEdgeConnected{},
      mGroupEdgeX{},
      mGroupEdgeY{},
      mGroupEdgeCount{},
      mGroupId{},
      mGroupCount(0),
      mWallListX{},
      mWallListY{},
      mWallListCount(0),
      mWalkableListX{},
      mWalkableListY{},
      mWalkableListCount(0),
      mSetParent{},
      mSetRank{},
      mSetEdgeConnected{},
      mWallsAreFinalized(0U),
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
  std::memset(mStackX, 0, sizeof(mStackX));
  std::memset(mStackY, 0, sizeof(mStackY));
  std::memset(mEdgeX, 0, sizeof(mEdgeX));
  std::memset(mEdgeY, 0, sizeof(mEdgeY));
  std::memset(mIsVisited, 0, sizeof(mIsVisited));
  std::memset(mIsMarked, 0, sizeof(mIsMarked));
  std::memset(mIsEdgeConnected, 0, sizeof(mIsEdgeConnected));
  std::memset(mGroup, 0, sizeof(mGroup));
  std::memset(mGroupIsEdgeConnected, 0, sizeof(mGroupIsEdgeConnected));
  std::memset(mGroupEdgeX, 0, sizeof(mGroupEdgeX));
  std::memset(mGroupEdgeY, 0, sizeof(mGroupEdgeY));
  std::memset(mGroupEdgeCount, 0, sizeof(mGroupEdgeCount));
  std::memset(mGroupId, 0xFF, sizeof(mGroupId));
  std::memset(mWallListX, 0, sizeof(mWallListX));
  std::memset(mWallListY, 0, sizeof(mWallListY));
  std::memset(mWalkableListX, 0, sizeof(mWalkableListX));
  std::memset(mWalkableListY, 0, sizeof(mWalkableListY));
  std::memset(mSetParent, 0xFF, sizeof(mSetParent));
  std::memset(mSetRank, 0, sizeof(mSetRank));
  std::memset(mSetEdgeConnected, 0, sizeof(mSetEdgeConnected));
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

void MazeGrid::ResetGrid() {
  std::memset(mIsWall, 0, sizeof(mIsWall));
  mStackCount = 0;
  mEdgeCount = 0;
  mGroupCount = 0;
  mWallListCount = 0;
  mWalkableListCount = 0;
  mPathLength = 0;
  mOpenHeapCount = 0;
  mWallsAreFinalized = 0U;
}

bool MazeGrid::FindPath(int pStartX, int pStartY, int pEndX, int pEndY) {
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

int MazeGrid::PathLength() const {
  return mPathLength;
}

bool MazeGrid::PathNode(int pIndex, int* pOutX, int* pOutY) const {
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

bool MazeGrid::IsWall(int pX, int pY) const {
  return !InBounds(pX, pY) || (mIsWall[pX][pY] != 0);
}

bool MazeGrid::IsEdge(int pX, int pY) const {
  if (!InBounds(pX, pY)) {
    return false;
  }
  return (pX == 0 || pY == 0 || pX == kGridWidth - 1 || pY == kGridHeight - 1);
}

bool MazeGrid::IsConnected_Slow(int pX1, int pY1, int pX2, int pY2) const {
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

bool MazeGrid::GetRandomWall(int& pX, int& pY) {
  if (mWallsAreFinalized == 0U) {
    FinalizeWalls();
  }
  if (mWallListCount <= 0) {
    return false;
  }
  const int aChoice = NextIndex(mWallListCount);
  pX = mWallListX[aChoice];
  pY = mWallListY[aChoice];
  return true;
}

bool MazeGrid::GetRandomWalkable(int& pX, int& pY) {
  if (mWallsAreFinalized == 0U) {
    FinalizeWalls();
  }
  if (mWalkableListCount <= 0) {
    return false;
  }
  const int aChoice = NextIndex(mWalkableListCount);
  pX = mWalkableListX[aChoice];
  pY = mWalkableListY[aChoice];
  return true;
}

void MazeGrid::FindIslands() {
  std::memset(mGroup, 0, sizeof(mGroup));
  std::memset(mGroupIsEdgeConnected, 0, sizeof(mGroupIsEdgeConnected));
  std::memset(mGroupEdgeCount, 0, sizeof(mGroupEdgeCount));
  std::memset(mGroupId, 0xFF, sizeof(mGroupId));
  mGroupCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsWall[aX][aY] != 0 || mGroupId[aX][aY] >= 0 || mGroupCount >= kMazeMaxGroups) {
        continue;
      }

      const int aGroupIndex = mGroupCount;
      ++mGroupCount;
      mStackCount = 0;
      mGroupId[aX][aY] = static_cast<short>(aGroupIndex);
      PushStackCell(aX, aY);

      while (mStackCount > 0) {
        --mStackCount;
        const int aVisitX = mStackX[mStackCount];
        const int aVisitY = mStackY[mStackCount];
        mGroup[aGroupIndex][aVisitX][aVisitY] = 1U;
        if (IsEdge(aVisitX, aVisitY)) {
          mGroupIsEdgeConnected[aGroupIndex] = 1U;
        }

        const int aNX[4] = {aVisitX - 1, aVisitX + 1, aVisitX, aVisitX};
        const int aNY[4] = {aVisitY, aVisitY, aVisitY - 1, aVisitY + 1};
        for (int aDir = 0; aDir < 4; ++aDir) {
          if (!InBounds(aNX[aDir], aNY[aDir]) || mIsWall[aNX[aDir]][aNY[aDir]] != 0 ||
              mGroupId[aNX[aDir]][aNY[aDir]] >= 0) {
            continue;
          }
          mGroupId[aNX[aDir]][aNY[aDir]] = static_cast<short>(aGroupIndex);
          PushStackCell(aNX[aDir], aNY[aDir]);
        }
      }
    }
  }
}

int MazeGrid::GroupCount() const {
  return mGroupCount;
}

int MazeGrid::AbsInt(int pValue) {
  return (pValue < 0) ? -pValue : pValue;
}

int MazeGrid::ToX(int pIndex) const {
  return pIndex & kGridMask;
}

int MazeGrid::ToY(int pIndex) const {
  return pIndex >> kGridShift;
}

bool MazeGrid::InBounds(int pX, int pY) const {
  return (pX >= 0 && pX < kGridWidth && pY >= 0 && pY < kGridHeight);
}

bool MazeGrid::IsWalkable(int pX, int pY) const {
  return InBounds(pX, pY) && (mIsWall[pX][pY] == 0);
}

int MazeGrid::HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const {
  return AbsInt(pEndX - ToX(pIndex)) + AbsInt(pEndY - ToY(pIndex));
}

int MazeGrid::ToIndex(int pX, int pY) const {
  return (pY << kGridShift) + pX;
}

void MazeGrid::ClearWalls() {
  std::memset(mIsWall, 0, sizeof(mIsWall));
  InvalidateWallLists();
}

void MazeGrid::SetWall(int pX, int pY, bool pIsWall) {
  if (!InBounds(pX, pY)) {
    return;
  }
  mIsWall[pX][pY] = pIsWall ? 1 : 0;
  InvalidateWallLists();
}

void MazeGrid::FinalizeWalls() {
  mWallListCount = 0;
  mWalkableListCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsWall[aX][aY] != 0) {
        mWallListX[mWallListCount] = aX;
        mWallListY[mWallListCount] = aY;
        ++mWallListCount;
      } else {
        mWalkableListX[mWalkableListCount] = aX;
        mWalkableListY[mWalkableListCount] = aY;
        ++mWalkableListCount;
      }
    }
  }

  mWallsAreFinalized = 1U;
}

void MazeGrid::InvalidateWallLists() {
  mWallListCount = 0;
  mWalkableListCount = 0;
  mWallsAreFinalized = 0U;
}

void MazeGrid::FillAxisOrder(int* pAxis, int pCount) {
  if (pAxis == nullptr || pCount <= 0) {
    return;
  }

  for (int aIndex = 0; aIndex < pCount; ++aIndex) {
    pAxis[aIndex] = aIndex;
  }

  for (int aIndex = pCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(pAxis[aIndex], pAxis[aSwapIndex]);
  }
}

void MazeGrid::FillStackAllCoords() {
  mStackCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      mStackX[mStackCount] = aX;
      mStackY[mStackCount] = aY;
      ++mStackCount;
    }
  }
}

void MazeGrid::ResetEdgeSearchState() {
  mStackCount = 0;
  mEdgeCount = 0;
  std::memset(mIsVisited, 0, sizeof(mIsVisited));
  std::memset(mIsMarked, 0, sizeof(mIsMarked));
  std::memset(mIsEdgeConnected, 0, sizeof(mIsEdgeConnected));
}

void MazeGrid::PushStackCell(int pX, int pY) {
  if (mStackCount >= kGridSize) {
    return;
  }
  mStackX[mStackCount] = pX;
  mStackY[mStackCount] = pY;
  ++mStackCount;
}

void MazeGrid::PushEdgeCell(int pX, int pY) {
  if (!InBounds(pX, pY) || !IsWall(pX, pY) || mIsMarked[pX][pY] != 0 || mEdgeCount >= kGridSize) {
    return;
  }
  mIsMarked[pX][pY] = 1;
  mEdgeX[mEdgeCount] = pX;
  mEdgeY[mEdgeCount] = pY;
  ++mEdgeCount;
}

void MazeGrid::RemoveEdgeCellAt(int pIndex) {
  if (pIndex < 0 || pIndex >= mEdgeCount) {
    return;
  }
  const int aLastIndex = mEdgeCount - 1;
  mEdgeX[pIndex] = mEdgeX[aLastIndex];
  mEdgeY[pIndex] = mEdgeY[aLastIndex];
  --mEdgeCount;
}

void MazeGrid::InitializeDisjointSets(int* pComponentCount) {
  int aComponents = 0;
  std::memset(mSetParent, 0xFF, sizeof(mSetParent));
  std::memset(mSetRank, 0, sizeof(mSetRank));
  std::memset(mSetEdgeConnected, 0, sizeof(mSetEdgeConnected));

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsWall[aX][aY] != 0) {
        continue;
      }
      const int aIndex = ToIndex(aX, aY);
      mSetParent[aIndex] = aIndex;
      mSetEdgeConnected[aIndex] = IsEdge(aX, aY) ? 1U : 0U;
      ++aComponents;
    }
  }

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mIsWall[aX][aY] != 0) {
        continue;
      }
      const int aIndex = ToIndex(aX, aY);
      if (aX > 0 && mIsWall[aX - 1][aY] == 0 && UnionSetRoots(aIndex, ToIndex(aX - 1, aY))) {
        --aComponents;
      }
      if (aY > 0 && mIsWall[aX][aY - 1] == 0 && UnionSetRoots(aIndex, ToIndex(aX, aY - 1))) {
        --aComponents;
      }
    }
  }

  if (pComponentCount != nullptr) {
    *pComponentCount = aComponents;
  }
}

int MazeGrid::FindSetRoot(int pIndex) {
  if (pIndex < 0 || pIndex >= kGridSize || mSetParent[pIndex] < 0) {
    return -1;
  }

  int aRoot = pIndex;
  while (mSetParent[aRoot] != aRoot) {
    aRoot = mSetParent[aRoot];
  }

  while (mSetParent[pIndex] != pIndex) {
    const int aParent = mSetParent[pIndex];
    mSetParent[pIndex] = aRoot;
    pIndex = aParent;
  }

  return aRoot;
}

bool MazeGrid::UnionSetRoots(int pIndexA, int pIndexB) {
  int aRootA = FindSetRoot(pIndexA);
  int aRootB = FindSetRoot(pIndexB);
  if (aRootA < 0 || aRootB < 0 || aRootA == aRootB) {
    return false;
  }

  if (mSetRank[aRootA] < mSetRank[aRootB]) {
    std::swap(aRootA, aRootB);
  }
  mSetParent[aRootB] = aRootA;
  mSetEdgeConnected[aRootA] =
      static_cast<unsigned char>((mSetEdgeConnected[aRootA] != 0U || mSetEdgeConnected[aRootB] != 0U) ? 1U : 0U);
  if (mSetRank[aRootA] == mSetRank[aRootB]) {
    ++mSetRank[aRootA];
  }
  return true;
}

void MazeGrid::OpenWallAndUnion(int pX, int pY, int* pComponentCount) {
  if (!InBounds(pX, pY) || mIsWall[pX][pY] == 0) {
    return;
  }

  SetWall(pX, pY, false);
  const int aIndex = ToIndex(pX, pY);
  mSetParent[aIndex] = aIndex;
  mSetRank[aIndex] = 0U;
  mSetEdgeConnected[aIndex] = IsEdge(pX, pY) ? 1U : 0U;

  if (pComponentCount != nullptr) {
    ++(*pComponentCount);
  }

  const int aNX[4] = {pX - 1, pX + 1, pX, pX};
  const int aNY[4] = {pY, pY, pY - 1, pY + 1};
  for (int aDir = 0; aDir < 4; ++aDir) {
    if (!InBounds(aNX[aDir], aNY[aDir]) || mIsWall[aNX[aDir]][aNY[aDir]] != 0) {
      continue;
    }
    if (UnionSetRoots(aIndex, ToIndex(aNX[aDir], aNY[aDir])) && pComponentCount != nullptr) {
      --(*pComponentCount);
    }
  }
}

bool MazeGrid::IsInteriorMazeCell(int pX, int pY) const {
  return InBounds(pX, pY) && !IsEdge(pX, pY) && ((pX & 1) == 1) && ((pY & 1) == 1);
}

bool MazeGrid::GetCellsForWall(int pWallX, int pWallY, int* pX1, int* pY1, int* pX2, int* pY2) const {
  if (pX1 == nullptr || pY1 == nullptr || pX2 == nullptr || pY2 == nullptr || !InBounds(pWallX, pWallY)) {
    return false;
  }

  int aX1 = -1;
  int aY1 = -1;
  int aX2 = -1;
  int aY2 = -1;
  if (((pWallX & 1) == 0) && ((pWallY & 1) == 1)) {
    aX1 = pWallX - 1;
    aY1 = pWallY;
    aX2 = pWallX + 1;
    aY2 = pWallY;
  } else if (((pWallX & 1) == 1) && ((pWallY & 1) == 0)) {
    aX1 = pWallX;
    aY1 = pWallY - 1;
    aX2 = pWallX;
    aY2 = pWallY + 1;
  } else {
    return false;
  }

  if (!IsInteriorMazeCell(aX1, aY1) || !IsInteriorMazeCell(aX2, aY2)) {
    return false;
  }

  *pX1 = aX1;
  *pY1 = aY1;
  *pX2 = aX2;
  *pY2 = aY2;
  return true;
}

void MazeGrid::PushPrimsFrontierWalls(int pCellX, int pCellY) {
  if (!IsInteriorMazeCell(pCellX, pCellY)) {
    return;
  }

  const int aStepX[4] = {-2, 2, 0, 0};
  const int aStepY[4] = {0, 0, -2, 2};
  for (int aDir = 0; aDir < 4; ++aDir) {
    const int aNextCellX = pCellX + aStepX[aDir];
    const int aNextCellY = pCellY + aStepY[aDir];
    if (!IsInteriorMazeCell(aNextCellX, aNextCellY) || mIsVisited[aNextCellX][aNextCellY] != 0) {
      continue;
    }
    PushEdgeCell(pCellX + (aStepX[aDir] >> 1), pCellY + (aStepY[aDir] >> 1));
  }
}

void MazeGrid::FindEdgeWalls(int pX, int pY) {
  ResetEdgeSearchState();
  if (!InBounds(pX, pY) || IsWall(pX, pY)) {
    return;
  }

  bool aTouchesEdge = false;
  mIsVisited[pX][pY] = 1;
  PushStackCell(pX, pY);

  while (mStackCount > 0) {
    --mStackCount;
    const int aX = mStackX[mStackCount];
    const int aY = mStackY[mStackCount];
    if (IsEdge(aX, aY)) {
      aTouchesEdge = true;
    }

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};
    for (int aDir = 0; aDir < 4; ++aDir) {
      if (!InBounds(aNX[aDir], aNY[aDir])) {
        continue;
      }
      if (IsWall(aNX[aDir], aNY[aDir])) {
        PushEdgeCell(aNX[aDir], aNY[aDir]);
        continue;
      }
      if (mIsVisited[aNX[aDir]][aNY[aDir]] != 0) {
        continue;
      }
      mIsVisited[aNX[aDir]][aNY[aDir]] = 1;
      PushStackCell(aNX[aDir], aNY[aDir]);
    }
  }

  if (aTouchesEdge) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      for (int aX = 0; aX < kGridWidth; ++aX) {
        if (mIsVisited[aX][aY] != 0) {
          mIsEdgeConnected[aX][aY] = 1;
        }
      }
    }
  }
}

void MazeGrid::BreakDownOneCellGroups() {
  unsigned char(*aWasWall)[kGridHeight] = mIsEdgeConnected;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      aWasWall[aX][aY] = static_cast<unsigned char>(mIsWall[aX][aY] != 0);
    }
  }

  for (int aY = 1; aY < kGridHeight - 1; ++aY) {
    for (int aX = 1; aX < kGridWidth - 1; ++aX) {
      if (aWasWall[aX - 1][aY - 1] == 0U || aWasWall[aX + 1][aY - 1] == 0U ||
          aWasWall[aX - 1][aY + 1] == 0U || aWasWall[aX + 1][aY + 1] == 0U) {
        continue;
      }
      SetWall(aX - 1, aY - 1, false);
      SetWall(aX + 1, aY - 1, false);
      SetWall(aX - 1, aY + 1, false);
      SetWall(aX + 1, aY + 1, false);
    }
  }
}

void MazeGrid::GenerateCustom() {
  ClearWalls();
  FillStackAllCoords();

  for (int aIndex = mStackCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(mStackX[aIndex], mStackX[aSwapIndex]);
    std::swap(mStackY[aIndex], mStackY[aSwapIndex]);
  }

  const int aWalls = (kGridSize >> 2);
  for (int aIndex = 0; aIndex < aWalls && aIndex < mStackCount; ++aIndex) {
    SetWall(mStackX[aIndex], mStackY[aIndex], true);
  }

  EnsureSingleConnectedOpenGroup();
}

void MazeGrid::EnsureSingleConnectedOpenGroup() {
  BreakDownOneCellGroups();
  int aComponentCount = 0;
  InitializeDisjointSets(&aComponentCount);
  if (aComponentCount <= 1) {
    return;
  }

  static constexpr int kBuildCap = 20000;
  for (int aIter = 0; aIter < kBuildCap; ++aIter) {
    if (aComponentCount <= 1) {
      return;
    }

    int aMergeCount = 0;
    int aGrowCount = 0;
    for (int aY = 0; aY < kGridHeight; ++aY) {
      for (int aX = 0; aX < kGridWidth; ++aX) {
        if (mIsWall[aX][aY] == 0) {
          continue;
        }

        int aRoots[4];
        int aRootCount = 0;
        const int aNX[4] = {aX - 1, aX + 1, aX, aX};
        const int aNY[4] = {aY, aY, aY - 1, aY + 1};
        for (int aDir = 0; aDir < 4; ++aDir) {
          if (!InBounds(aNX[aDir], aNY[aDir]) || mIsWall[aNX[aDir]][aNY[aDir]] != 0) {
            continue;
          }

          const int aRoot = FindSetRoot(ToIndex(aNX[aDir], aNY[aDir]));
          if (aRoot < 0) {
            continue;
          }

          bool aSeenRoot = false;
          for (int aRootIndex = 0; aRootIndex < aRootCount; ++aRootIndex) {
            if (aRoots[aRootIndex] == aRoot) {
              aSeenRoot = true;
              break;
            }
          }
          if (!aSeenRoot) {
            aRoots[aRootCount] = aRoot;
            ++aRootCount;
          }
        }

        if (aRootCount == 2 && aMergeCount < kGridSize) {
          mEdgeX[aMergeCount] = aX;
          mEdgeY[aMergeCount] = aY;
          ++aMergeCount;
        } else if (aRootCount >= 1 && aGrowCount < kGridSize) {
          mStackX[aGrowCount] = aX;
          mStackY[aGrowCount] = aY;
          ++aGrowCount;
        }
      }
    }

    if (aMergeCount > 0) {
      const int aChoice = NextIndex(aMergeCount);
      OpenWallAndUnion(mEdgeX[aChoice], mEdgeY[aChoice], &aComponentCount);
      continue;
    }
    if (aGrowCount > 0) {
      const int aChoice = NextIndex(aGrowCount);
      OpenWallAndUnion(mStackX[aChoice], mStackY[aChoice], &aComponentCount);
      continue;
    }
    break;
  }
}

void MazeGrid::GeneratePrims() {
  std::memset(mIsVisited, 0, sizeof(mIsVisited));
  std::memset(mIsMarked, 0, sizeof(mIsMarked));
  std::memset(mIsEdgeConnected, 0, sizeof(mIsEdgeConnected));
  mStackCount = 0;
  mEdgeCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      SetWall(aX, aY, true);
    }
  }

  const int aCellCountX = (kGridWidth - 1) / 2;
  const int aCellCountY = (kGridHeight - 1) / 2;
  const int aStartX = 1 + (NextIndex(aCellCountX) << 1);
  const int aStartY = 1 + (NextIndex(aCellCountY) << 1);
  mIsVisited[aStartX][aStartY] = 1;
  SetWall(aStartX, aStartY, false);
  PushPrimsFrontierWalls(aStartX, aStartY);

  while (mEdgeCount > 0) {
    const int aEdgeIndex = NextIndex(mEdgeCount);
    const int aWallX = mEdgeX[aEdgeIndex];
    const int aWallY = mEdgeY[aEdgeIndex];
    RemoveEdgeCellAt(aEdgeIndex);

    int aX1 = -1;
    int aY1 = -1;
    int aX2 = -1;
    int aY2 = -1;
    if (!GetCellsForWall(aWallX, aWallY, &aX1, &aY1, &aX2, &aY2)) {
      continue;
    }

    const bool aVisited1 = (mIsVisited[aX1][aY1] != 0);
    const bool aVisited2 = (mIsVisited[aX2][aY2] != 0);
    if (aVisited1 == aVisited2) {
      continue;
    }

    const int aTargetX = aVisited1 ? aX2 : aX1;
    const int aTargetY = aVisited1 ? aY2 : aY1;
    SetWall(aWallX, aWallY, false);
    SetWall(aTargetX, aTargetY, false);
    mIsVisited[aTargetX][aTargetY] = 1;
    PushPrimsFrontierWalls(aTargetX, aTargetY);
  }
}

void MazeGrid::InitializeKruskals() {
  std::memset(mIsVisited, 0, sizeof(mIsVisited));
  std::memset(mIsMarked, 0, sizeof(mIsMarked));
  std::memset(mIsEdgeConnected, 0, sizeof(mIsEdgeConnected));
  std::memset(mGroupId, 0xFF, sizeof(mGroupId));
  mStackCount = 0;
  mEdgeCount = 0;
  mGroupCount = 0;

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      SetWall(aX, aY, true);
    }
  }

  for (int aY = 1; aY < kGridHeight; aY += 2) {
    for (int aX = 1; aX < kGridWidth; aX += 2) {
      if (!IsInteriorMazeCell(aX, aY) || mGroupCount >= kMazeMaxGroups) {
        continue;
      }
      SetWall(aX, aY, false);
      mGroupId[aX][aY] = static_cast<short>(mGroupCount);
      ++mGroupCount;

      if (IsInteriorMazeCell(aX + 2, aY)) {
        PushEdgeCell(aX + 1, aY);
      }
      if (IsInteriorMazeCell(aX, aY + 2)) {
        PushEdgeCell(aX, aY + 1);
      }
    }
  }
}

void MazeGrid::ExecuteKruskals() {
  InitializeKruskals();

  while (mEdgeCount > 0) {
    const int aEdgeIndex = NextIndex(mEdgeCount);
    const int aWallX = mEdgeX[aEdgeIndex];
    const int aWallY = mEdgeY[aEdgeIndex];
    RemoveEdgeCellAt(aEdgeIndex);

    int aX1 = -1;
    int aY1 = -1;
    int aX2 = -1;
    int aY2 = -1;
    if (!GetCellsForWall(aWallX, aWallY, &aX1, &aY1, &aX2, &aY2)) {
      continue;
    }

    const int aGroup1 = static_cast<int>(mGroupId[aX1][aY1]);
    const int aGroup2 = static_cast<int>(mGroupId[aX2][aY2]);
    if (aGroup1 < 0 || aGroup2 < 0 || aGroup1 == aGroup2) {
      continue;
    }

    SetWall(aWallX, aWallY, false);
    for (int aY = 1; aY < kGridHeight; aY += 2) {
      for (int aX = 1; aX < kGridWidth; aX += 2) {
        if (mGroupId[aX][aY] == aGroup2) {
          mGroupId[aX][aY] = static_cast<short>(aGroup1);
        }
      }
    }
  }
}

bool MazeGrid::OpenNodeLess(int pPosA, int pPosB) const {
  const int aIndexA = mOpenHeap[pPosA];
  const int aIndexB = mOpenHeap[pPosB];
  const int aFA = mFScore[aIndexA];
  const int aFB = mFScore[aIndexB];
  if (aFA != aFB) {
    return aFA < aFB;
  }
  return mGScore[aIndexA] < mGScore[aIndexB];
}

void MazeGrid::SwapOpenNodes(int pPosA, int pPosB) {
  std::swap(mOpenHeap[pPosA], mOpenHeap[pPosB]);
  const int aIndexA = mOpenHeap[pPosA];
  const int aIndexB = mOpenHeap[pPosB];
  mOpenHeapPosByIndex[aIndexA] = static_cast<short>(pPosA);
  mOpenHeapPosByIndex[aIndexB] = static_cast<short>(pPosB);
}

void MazeGrid::HeapifyUp(int pPos) {
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

void MazeGrid::HeapifyDown(int pPos) {
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

void MazeGrid::PushOpenNode(int pIndex) {
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

void MazeGrid::UpdateOpenNodePriority(int pIndex) {
  if (mNodeStateByIndex[pIndex] != 1U) {
    return;
  }
  const int aPos = mOpenHeapPosByIndex[pIndex];
  if (aPos < 0 || aPos >= mOpenHeapCount) {
    return;
  }
  HeapifyUp(aPos);
}

int MazeGrid::RemoveOpenMin() {
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

bool MazeGrid::IsInClosedList(int pIndex) const {
  return mNodeStateByIndex[pIndex] == 2U;
}

bool MazeGrid::IsInOpenList(int pIndex) const {
  return mNodeStateByIndex[pIndex] == 1U;
}

void MazeGrid::ReconstructPath(int pEndIndex) {
  int aReverseCount = 0;

  int aX = ToX(pEndIndex);
  int aY = ToY(pEndIndex);
  while (aReverseCount < kGridSize && aX >= 0 && aY >= 0) {
    mStackX[aReverseCount] = aX;
    mStackY[aReverseCount] = aY;
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
    mPathX[mPathLength] = mStackX[aIndex];
    mPathY[mPathLength] = mStackY[aIndex];
    ++mPathLength;
  }
}

}  // namespace peanutbutter::maze
