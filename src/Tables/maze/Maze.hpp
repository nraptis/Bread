#ifndef BREAD_SRC_MAZE_MAZE_HPP_
#define BREAD_SRC_MAZE_MAZE_HPP_

#include <cstdint>

#include "../../PeanutButter.hpp"
#include "../rng/Shuffler.hpp"

namespace peanutbutter::fast_rand {
class FastRand;
}

namespace peanutbutter::maze {

class Maze : public peanutbutter::rng::Shuffler {
 public:
  enum class FlushAccountingMode {
    kRegular,
    kStalled
  };

  struct RuntimeStats {
    std::uint64_t mVictories = 0U;
    std::uint64_t mDeaths = 0U;
    std::uint64_t mFlush = 0U;
    std::uint64_t mEmptyFlush = 0U;
    std::uint64_t mTilesFlushedRegular = 0U;
    std::uint64_t mTilesFlushedStalled = 0U;
    std::uint64_t mTilesPaintedValidScenario = 0U;
    std::uint64_t mInconsistentStateA = 0U;
    std::uint64_t mInconsistentStateB = 0U;
    std::uint64_t mSimulationStallCataclysmic = 0U;
    std::uint64_t mSimulationStallApocalypse = 0U;
  };

  static constexpr int kGridWidth = 32;
  static constexpr int kGridHeight = 32;
  static constexpr int kGridSize = kGridWidth * kGridHeight;
  static constexpr int kGridShift = 5;
  static constexpr int kGridMask = kGridWidth - 1;
  static constexpr int kMazeMaxGroups = 256;
  static constexpr int kSeedBufferCapacity = PASSWORD_BALLOONED_SIZE;
  static constexpr int kMazeRespawnActionThreshold = 16;

  Maze();
  ~Maze() override = default;

  virtual void Seed(unsigned char* pPassword, int pPasswordLength) override = 0;

  bool FindPath(int pStartX, int pStartY, int pEndX, int pEndY);
  RuntimeStats GetRuntimeStats() const;
  int PathLength() const;
  bool PathNode(int pIndex, int* pOutX, int* pOutY) const;
  bool IsWall(int pX, int pY) const;
  bool IsEdge(int pX, int pY) const;
  bool IsConnected_Slow(int pX1, int pY1, int pX2, int pY2) const;
  bool GetRandomWall(int& pX, int& pY);
  bool GetRandomWalkable(int& pX, int& pY);
  void FinalizeWalls();

 protected:
  static int AbsInt(int pValue);
  int ToX(int pIndex) const;
  int ToY(int pIndex) const;
  bool InBounds(int pX, int pY) const;
  bool IsWalkable(int pX, int pY) const;
  int HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const;
  int ToIndex(int pX, int pY) const;
  virtual int NextIndex(int pLimit) = 0;
  void InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength);
  void ShuffleSeedBuffer(peanutbutter::fast_rand::FastRand* pFastRand);
  void Reset();
  bool PaintWalkableFromSeed();
  bool RepaintFromSeed(int pX, int pY);
  void ApocalypseScenario();
  void SetFlushAccountingMode(FlushAccountingMode pMode);
  void ClearWalls();
  void ClearByteCells();
  void SetWall(int pX, int pY, bool pIsWall);
  void Repaint(int pX, int pY, unsigned char pValue);
  void Flush(int pX, int pY);
  void SetByteCell(int pX, int pY, unsigned char pByte, bool pIsByte);
  void Flush();
  void FillStackAllCoords();
  void FindEdgeWalls(int pX, int pY);
  void BreakDownOneCellGroups();
  void EnsureSingleConnectedOpenGroup();
  void GeneratePrims();
  void InitializeKruskals();
  void ExecuteKruskals();

  int mIsWall[kGridWidth][kGridHeight];
  int mIsByte[kGridWidth][kGridHeight];
  unsigned char mByte[kGridWidth][kGridHeight];
  RuntimeStats mRuntimeStats;
  int mStackX[kGridSize];
  int mStackY[kGridSize];
  int mStackCount;
  int mEdgeX[kGridSize];
  int mEdgeY[kGridSize];
  int mEdgeCount;
  unsigned char mIsVisited[kGridWidth][kGridHeight];
  unsigned char mIsMarked[kGridWidth][kGridHeight];
  unsigned char mIsEdgeConnected[kGridWidth][kGridHeight];
  unsigned char mGroup[kMazeMaxGroups][kGridWidth][kGridHeight];
  unsigned char mGroupIsEdgeConnected[kMazeMaxGroups];
  int mGroupEdgeX[kMazeMaxGroups][kGridSize];
  int mGroupEdgeY[kMazeMaxGroups][kGridSize];
  int mGroupEdgeCount[kMazeMaxGroups];
  short mGroupId[kGridWidth][kGridHeight];
  int mGroupCount;
  int mWallListX[kGridSize];
  int mWallListY[kGridSize];
  int mWallListCount;
  int mWalkableListX[kGridSize];
  int mWalkableListY[kGridSize];
  int mWalkableListCount;
  int mSetParent[kGridSize];
  unsigned char mSetRank[kGridSize];
  unsigned char mSetEdgeConnected[kGridSize];
  unsigned char mWallsAreFinalized;
  FlushAccountingMode mFlushAccountingMode;
  int mFlushX[kGridWidth];
  int mFlushY[kGridHeight];

 private:
  void InvalidateWallLists();
  void FillAxisOrder(int* pAxis, int pCount);
  void ResetEdgeSearchState();
  void PushStackCell(int pX, int pY);
  void PushEdgeCell(int pX, int pY);
  void RemoveEdgeCellAt(int pIndex);
  void PushPrimsFrontierWalls(int pCellX, int pCellY);
  void InitializeDisjointSets(int* pComponentCount);
  int FindSetRoot(int pIndex);
  bool UnionSetRoots(int pIndexA, int pIndexB);
  void OpenWallAndUnion(int pX, int pY, int* pComponentCount);
  bool IsInteriorMazeCell(int pX, int pY) const;
  bool GetCellsForWall(int pWallX, int pWallY, int* pX1, int* pY1, int* pX2, int* pY2) const;
  bool OpenNodeLess(int pPosA, int pPosB) const;
  void SwapOpenNodes(int pPosA, int pPosB);
  void HeapifyUp(int pPos);
  void HeapifyDown(int pPos);
  void PushOpenNode(int pIndex);
  void UpdateOpenNodePriority(int pIndex);
  int RemoveOpenMin();
  bool IsInClosedList(int pIndex) const;
  bool IsInOpenList(int pIndex) const;
  void ReconstructPath(int pEndIndex);

  int mPathX[kGridSize];
  int mPathY[kGridSize];
  int mPathLength;
  int mParentX[kGridSize];
  int mParentY[kGridSize];
  int mGScore[kGridSize];
  int mFScore[kGridSize];
  int mOpenHeap[kGridSize];
  int mOpenHeapCount;
  short mOpenHeapPosByIndex[kGridSize];
  unsigned char mNodeStateByIndex[kGridSize];
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZE_HPP_
