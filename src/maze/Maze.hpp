#ifndef BREAD_SRC_MAZE_MAZE_HPP_
#define BREAD_SRC_MAZE_MAZE_HPP_

#include "src/expansion/key_expansion/PasswordExpander.hpp"
#include "src/rng/Counter.hpp"
#include "src/rng/Digest.hpp"

namespace bread::maze {

class Maze : public bread::rng::Digest {
 public:
  static constexpr int kGridWidth = 32;
  static constexpr int kGridHeight = 32;
  static constexpr int kGridSize = kGridWidth * kGridHeight;
  static constexpr int kGridShift = 5;
  static constexpr int kGridMask = kGridWidth - 1;
  static constexpr int kSeedBufferCapacity = 100000000;

  Maze(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
       bread::rng::Counter* pCounter);
  ~Maze() override = default;

  using bread::rng::Digest::Get;
  void Seed(unsigned char* pPassword, int pPasswordLength) override;
  void Get(unsigned char* pDestination, int pDestinationLength) override;
  unsigned char Get();

  void Build();
  bool FindPath(int pStartX, int pStartY, int pEndX, int pEndY);
  int PathLength() const;
  bool PathNode(int pIndex, int* pOutX, int* pOutY) const;
  bool IsWall(int pX, int pY) const;

 private:
  static int AbsInt(int pValue);
  int ToX(int pIndex) const;
  int ToY(int pIndex) const;
  bool InBounds(int pX, int pY) const;
  bool IsWalkable(int pX, int pY) const;
  int HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const;
  int ToIndex(int pX, int pY) const;
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

  int NextByte();
  int NextIndex(int pLimit);
  void FillStackAllCoords();
  void ShuffleStack();
  void SetInitialWalls();
  int CollectOpenComponents(bool pMarkLabels, int* pComponentLabel, int* pComponentCount, bool* pTouchesEdge);
  bool OpenWallForEnclosedComponent(int pComponentId, const int* pComponentLabel);
  bool OpenWallToMergeComponents(const int* pComponentLabel, int pComponentCount);
  void EnsureSimpleCornerPath();
  void EncodeMazeToBuffer();

  int mWall[kGridWidth][kGridHeight];

  int mStackX[kGridSize];
  int mStackY[kGridSize];
  int mStackCount;

  int mSafeX[kGridSize];
  int mSafeY[kGridSize];
  unsigned char mSafeByte[kGridSize];
  int mSafeCount;

  int mRobotX;
  int mRobotY;
  int mCheeseX;
  int mCheeseY;

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

  bread::expansion::key_expansion::PasswordExpander* mPasswordExpander;
  bread::rng::Counter* mCounter;

  unsigned char mBuffer[kSeedBufferCapacity];
  int mBufferLength;
  int mGetCursor;
  int mPutCursor;
};

}  // namespace bread::maze

#endif  // BREAD_SRC_MAZE_MAZE_HPP_
