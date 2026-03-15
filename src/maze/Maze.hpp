#ifndef BREAD_SRC_MAZE_MAZE_HPP_
#define BREAD_SRC_MAZE_MAZE_HPP_

#include <cstdint>

#include "src/core/Bread.hpp"
#include "src/rng/Digest.hpp"

namespace bread::fast_rand {
class FastRand;
}

namespace bread::rng {
class Counter;
}

namespace bread::maze {

class Maze : public bread::rng::Digest {
 public:
  struct RuntimeStats {
    std::uint64_t mIsRobot = 0U;
    std::uint64_t mIsDolphin = 0U;
    std::uint64_t mFlush = 0U;
    std::uint64_t mEmptyFlush = 0U;
  };

  static constexpr int kGridWidth = 32;
  static constexpr int kGridHeight = 32;
  static constexpr int kGridSize = kGridWidth * kGridHeight;
  static constexpr int kGridShift = 5;
  static constexpr int kGridMask = kGridWidth - 1;
  static constexpr int kSeedBufferCapacity = PASSWORD_EXPANDED_SIZE;

  Maze();
  ~Maze() override = default;

  using bread::rng::Digest::Get;
  virtual void Seed(unsigned char* pPassword, int pPasswordLength) override = 0;
  void Get(unsigned char* pDestination, int pDestinationLength) override;
  unsigned char Get();

  bool FindPath(int pStartX, int pStartY, int pEndX, int pEndY);
  RuntimeStats GetRuntimeStats() const;
  int PathLength() const;
  bool PathNode(int pIndex, int* pOutX, int* pOutY) const;
  bool IsWall(int pX, int pY) const;
  bool IsEdge(int pX, int pY) const;
  bool IsConnected_Slow(int pX1, int pY1, int pX2, int pY2) const;
  bool IsConnectedToEdge(int pX, int pY) const;

 protected:
  static int AbsInt(int pValue);
  int ToX(int pIndex) const;
  int ToY(int pIndex) const;
  bool InBounds(int pX, int pY) const;
  bool IsWalkable(int pX, int pY) const;
  int HeuristicCostByIndex(int pIndex, int pEndX, int pEndY) const;
  int ToIndex(int pX, int pY) const;
  void InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength, bread::rng::Counter* pCounter);
  bool SeedCanDequeue() const;
  unsigned char SeedDequeue();
  void EnqueueByte(unsigned char pByte);
  void ShuffleSeedBuffer(bread::fast_rand::FastRand* pFastRand);
  void ClearWalls();
  void ClearByteCells();
  void SetWall(int pX, int pY, bool pIsWall);
  void SetByteCell(int pX, int pY, unsigned char pByte, bool pIsByte);
  void Flush();

  int mIsWall[kGridWidth][kGridHeight];
  int mIsByte[kGridWidth][kGridHeight];
  unsigned char mByte[kGridWidth][kGridHeight];
  unsigned char mSeedBuffer[kSeedBufferCapacity];
  unsigned char* mResultBuffer;
  unsigned int mResultBufferReadIndex;
  unsigned int mResultBufferWriteIndex;
  unsigned int mResultBufferLength;
  unsigned int mSeedBytesRemaining;
  std::uint64_t mResultBufferWriteProgress;
  RuntimeStats mRuntimeStats;

 private:
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

}  // namespace bread::maze

#endif  // BREAD_SRC_MAZE_MAZE_HPP_
