#ifndef BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_
#define BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_

#include <array>
#include <cstdint>

#include "../fast_rand/FastRand.hpp"
#include "Maze.hpp"
#include "MazeHelpers.hpp"
#include "MazePolicy.hpp"

namespace peanutbutter::maze {

class MazeDirector final : public Maze {
 public:
  struct ProbeStats {
    std::uint64_t mSimulationCount = 0U;
    std::uint64_t mRobotLifeCount = 0U;
    std::uint64_t mTotalRobotWalk = 0U;
    std::uint64_t mShortestRobotWalk = 0U;
    std::uint64_t mLongestRobotWalk = 0U;
    std::uint64_t mSharkKillCount = 0U;
    std::uint64_t mTotalSharkMovesBeforeKill = 0U;
  };

  static constexpr int kGameCount = peanutbutter::maze::kGameCount;

  MazeDirector();
  ~MazeDirector() override = default;

  void Seed(unsigned char* pPassword, int pPasswordLength) override;
  void SetGame(int pGameIndex);
  int CurrentGame() const;
  const char* GetName() const;
  void SetProbeConfig(const ProbeConfig& pConfig);
  ProbeConfig GetProbeConfig() const;
  ProbeStats GetProbeStats() const;

 protected:
  int NextIndex(int pLimit) override;

 private:
  static constexpr int kSimulationStallThreshold = 4;

  int ClampConfiguredCount(int pConfiguredCount, int pMaximum) const;
  GenerationMode ResolveGenerationMode() const;
  void ResetPulseState();
  bool IsTileOccupied(int pX,
                      int pY,
                      int pIgnoreRobotIndex = -1,
                      int pIgnoreCheeseIndex = -1,
                      int pIgnoreSharkIndex = -1) const;
  void FinalizeRobotLife(int pRobotIndex);
  void FinalizePulseStats();
  bool RespawnRobot(int pRobotIndex);
  bool RespawnCheese(int pCheeseIndex);
  bool RespawnShark(int pSharkIndex);
  bool InitializePulseState();
  int FindSharkAt(int pX, int pY) const;
  void MoveShark(helpers::MazeShark& pShark, int pSharkIndex);
  helpers::MazeCheese* PickRandomValidCheese();
  bool StoreRobotPath(helpers::MazeRobot* pRobot, helpers::MazeCheese* pCheese);
  bool AssignPathableCheese(helpers::MazeRobot* pRobot);
  bool RepaintOrFlushTile(int pX, int pY);
  void RepaintRobotSquare(int pCenterX, int pCenterY);
  void ApplyRobotPowerUp(int pRobotIndex);
  bool CaptureCheese(int pRobotIndex);
  void RecordRobotAction(int* pActionCount, int* pOutActions);
  void TriggerConfusedRobot(helpers::MazeRobot* pRobot);
  void KillRobotAndShark(int pRobotIndex, int pSharkIndex);
  void PickDistinctRows(int* pRows, int pRowCount);
  void PickDistinctCols(int* pCols, int pColCount);
  void TriggerSpecialEvents();
  bool MoveSharks();
  void CollideRobotsWithSharks();
  bool ManageRobots(bool* pRobotCanMove, int* pActionCount, int* pOutActions);
  bool MoveRobots(const bool* pRobotCanMove, int* pActionCount, int* pOutActions);
  bool ManageCheese();
  bool Generate();
  bool PathingLoopPulse(int* pOutActions);
  bool SimulationMainLoop();
  void Simulation();
  void Build();

  peanutbutter::fast_rand::FastRand mFastRand;
  ProbeConfig mProbeConfig;
  ProbeStats mProbeStats;
  int mGameIndex;
  const char* mGameName;
  std::array<helpers::MazeRobot, helpers::kMaxRobots> mRobots;
  std::array<helpers::MazeCheese, helpers::kMaxCheeses> mCheeses;
  std::array<helpers::MazeShark, helpers::kMaxSharks> mSharks;
  unsigned char mIsShark[kGridWidth][kGridHeight];
  unsigned char mPowerUpType[kGridWidth][kGridHeight];
  int mRobotWalks[helpers::kMaxRobots];
  int mSharkMovesSinceKill[helpers::kMaxSharks];
  int mSharkMoveCandidateX[4];
  int mSharkMoveCandidateY[4];
  int mSharkMoveCandidateCount;
  bool mRobotLifeActive[helpers::kMaxRobots];
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_
