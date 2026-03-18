#ifndef BREAD_SRC_MAZE_MAZE_HPP_
#define BREAD_SRC_MAZE_MAZE_HPP_

#include <cstdint>

#include "../../PeanutButter.hpp"
#include "../rng/Shuffler.hpp"
#include "MazeCharacters.hpp"
#include "MazeGrid.hpp"

namespace peanutbutter::fast_rand {
class FastRand;
}

namespace peanutbutter::maze {

class MazeSpecialEvents;

class Maze : public peanutbutter::rng::Shuffler, protected MazeGrid {
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
    std::uint64_t mInconsistentStateC = 0U;
    std::uint64_t mInconsistentStateD = 0U;
    std::uint64_t mInconsistentStateE = 0U;
    std::uint64_t mInconsistentStateF = 0U;
    std::uint64_t mInconsistentStateG = 0U;
    std::uint64_t mInconsistentStateH = 0U;
    std::uint64_t mInconsistentStateI = 0U;
    std::uint64_t mSimulationStallCataclysmic = 0U;
    std::uint64_t mSimulationStallApocalypse = 0U;
    std::uint64_t mStarBurst = 0U;
    std::uint64_t mChaosStorm = 0U;
    std::uint64_t mCometTrailsHorizontal = 0U;
    std::uint64_t mCometTrailsVertical = 0U;
    std::uint64_t mDolphinCheeseTriages = 0U;
    std::uint64_t mDolphinSharkCollisions = 0U;
    std::uint64_t mDolphinSharkKills = 0U;
    std::uint64_t mDolphinDeaths = 0U;
  };

  static constexpr int kGridWidth = MazeGrid::kGridWidth;
  static constexpr int kGridHeight = MazeGrid::kGridHeight;
  static constexpr int kGridSize = MazeGrid::kGridSize;
  static constexpr int kGridShift = MazeGrid::kGridShift;
  static constexpr int kGridMask = MazeGrid::kGridMask;
  static constexpr int kMazeMaxGroups = MazeGrid::kMazeMaxGroups;
  static constexpr int kSeedBufferCapacity = PASSWORD_BALLOONED_SIZE;

  Maze();
  ~Maze() override = default;

  virtual void Seed(unsigned char* pPassword, int pPasswordLength) override = 0;

  using MazeGrid::FindPath;
  using MazeGrid::GetRandomWall;
  using MazeGrid::GetRandomWalkable;
  using MazeGrid::IsConnected_Slow;
  using MazeGrid::IsEdge;
  using MazeGrid::IsWall;
  using MazeGrid::PathLength;
  using MazeGrid::PathNode;
  using MazeGrid::FinalizeWalls;

  RuntimeStats GetRuntimeStats() const;

 protected:
  using MazeGrid::AbsInt;
  using MazeGrid::BreakDownOneCellGroups;
  using MazeGrid::ClearWalls;
  using MazeGrid::EnsureSingleConnectedOpenGroup;
  using MazeGrid::ExecuteKruskals;
  using MazeGrid::FillAxisOrder;
  using MazeGrid::FillStackAllCoords;
  using MazeGrid::FindEdgeWalls;
  using MazeGrid::GeneratePrims;
  using MazeGrid::HeuristicCostByIndex;
  using MazeGrid::InBounds;
  using MazeGrid::InitializeKruskals;
  using MazeGrid::IsWalkable;
  using MazeGrid::ResetGrid;
  using MazeGrid::SetWall;
  using MazeGrid::ToIndex;
  using MazeGrid::ToX;
  using MazeGrid::ToY;
  using MazeGrid::mEdgeCount;
  using MazeGrid::mEdgeX;
  using MazeGrid::mEdgeY;
  using MazeGrid::mGroupCount;
  using MazeGrid::mIsMarked;
  using MazeGrid::mIsVisited;
  using MazeGrid::mIsWall;
  using MazeGrid::mStackCount;
  using MazeGrid::mStackX;
  using MazeGrid::mStackY;
  using MazeGrid::mWalkableListCount;
  using MazeGrid::mWalkableListX;
  using MazeGrid::mWalkableListY;
  using MazeGrid::mWallListCount;
  using MazeGrid::mWallsAreFinalized;

  virtual int NextIndex(int pLimit) override = 0;
  void InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength);
  void ShuffleSeedBuffer(peanutbutter::fast_rand::FastRand* pFastRand);
  void Reset();
  bool PaintWalkableFromSeed();
  bool RepaintFromSeed(int pX, int pY);
  void ApocalypseScenario();
  void SetFlushAccountingMode(FlushAccountingMode pMode);
  void ClearByteCells();
  void Repaint(int pX, int pY, unsigned char pValue);
  void Flush(int pX, int pY);
  void SetByteCell(int pX, int pY, unsigned char pByte, bool pIsByte);
  void Flush();
  void ResetCharacterLists(int pRobotCount, int pCheeseCount, int pSharkCount, int pDolphinCount);

  int mIsByte[kGridWidth][kGridHeight];
  unsigned char mByte[kGridWidth][kGridHeight];
  helpers::Robot mRobotStorage[helpers::kMaxRobots];
  helpers::Cheese mCheeseStorage[helpers::kMaxCheeses];
  helpers::Shark mSharkStorage[helpers::kMaxSharks];
  helpers::Dolphin mDolphinStorage[helpers::kMaxDolphins];
  helpers::Robot* mRobotList[helpers::kMaxRobots];
  int mRobotListCount;
  helpers::Cheese* mCheeseList[helpers::kMaxCheeses];
  int mCheeseListCount;
  helpers::Shark* mSharkList[helpers::kMaxSharks];
  int mSharkListCount;
  helpers::Dolphin* mDolphinList[helpers::kMaxDolphins];
  int mDolphinListCount;
  RuntimeStats mRuntimeStats;
  FlushAccountingMode mFlushAccountingMode;
  int mFlushX[kGridWidth];
  int mFlushY[kGridHeight];

  friend class MazeSpecialEvents;
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZE_HPP_
