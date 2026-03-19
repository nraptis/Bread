#ifndef BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_
#define BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_

#include <cstdint>

#include "../fast_rand/FastRand.hpp"
#include "Maze.hpp"
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
  void ResetPulseOutcomeFlags();
  bool IsTileOccupied(int pX,
                      int pY,
                      int pIgnoreRobotIndex = -1,
                      int pIgnoreCheeseIndex = -1,
                      int pIgnoreSharkIndex = -1,
                      int pIgnoreDolphinIndex = -1) const;
  bool Generate();
  void Simulation();
  bool SimulationMainLoop();
  bool PathingLoopPulse();
  void HandleSpecialEvents();
  bool MoveSharks();
  bool MoveDolphins();
  bool ResolveRobotVictories();
  void MarkRobotSharkCollisions();
  void MarkDolphinSharkCollisions();
  bool ResolveCapturedCheeses();
  bool ResolveDeadEntities();
  bool ResolveRobotRepaths();
  bool MoveRobots();
  void CollectPowerUps();
  void UpdateRobots();
  bool ManageCheese();
  void FinalizeRobotLife(int pRobotIndex);
  void FinalizePulseStats();
  bool RespawnRobot(int pRobotIndex, bool pMarkRecentlyRevived);
  bool RespawnCheese(int pCheeseIndex);
  bool RespawnShark(int pSharkIndex, bool pMarkRecentlyRevived);
  bool RespawnDolphin(int pDolphinIndex, bool pMarkRecentlyRevived);
  bool InitializePulseState();
  int FindSharkAt(int pX, int pY) const;
  int FindCheeseAt(int pX, int pY) const;
  int FindRobotIndex(const helpers::MazeRobot* pRobot) const;
  void MoveShark(helpers::MazeShark& pShark, int pSharkIndex);
  void MoveDolphin(helpers::MazeDolphin& pDolphin, int pDolphinIndex);
  helpers::MazeCheese* PickRandomCheese(int pIgnoreCheeseIndex = -1);
  void ClearRobotTarget(helpers::MazeRobot* pRobot);
  bool StoreRobotPath(helpers::MazeRobot* pRobot, helpers::MazeCheese* pCheese);
  bool AssignPathableCheese(helpers::MazeRobot* pRobot, int pIgnoreCheeseIndex = -1);
  bool CheckRobotConsistency(const helpers::MazeRobot& pRobot) const;
  bool RepaintOrFlushTile(int pX, int pY);
  void RepaintRobotSquare(int pCenterX, int pCenterY);
  int DurationForPowerUp(helpers::PowerUpType pType) const;
  bool TryTeleportRobotNearTargetCheese(helpers::MazeRobot* pRobot);
  bool IsRobotInvincible(const helpers::MazeRobot& pRobot) const;
  void ApplyPowerUpEffects();
  bool MarkRobotVictory(int pRobotIndex, bool* pOutFailure = nullptr);
  bool MarkDolphinCheeseSteal(int pDolphinIndex);
  void MarkDolphinAndSharkDead(int pDolphinIndex, int pSharkIndex);
  void MarkRobotAndSharkDead(int pRobotIndex, int pSharkIndex);
  void PickDistinctRows(int* pRows, int pRowCount);
  void PickDistinctCols(int* pCols, int pColCount);
  void Build();

  peanutbutter::fast_rand::FastRand mFastRand;
  ProbeConfig mProbeConfig;
  ProbeStats mProbeStats;
  int mGameIndex;
  const char* mGameName;
  unsigned char mIsShark[kGridWidth][kGridHeight];
  unsigned char mIsDolphin[kGridWidth][kGridHeight];
  unsigned char mPowerUpType[kGridWidth][kGridHeight];
  int mRobotWalks[helpers::kMaxRobots];
  int mSharkMovesSinceKill[helpers::kMaxSharks];
  int mSharkMoveCandidateX[4];
  int mSharkMoveCandidateY[4];
  int mSharkMoveCandidateCount;
  int mMainLoopIterationVictoryCount;
  bool mRobotLifeActive[helpers::kMaxRobots];
  bool mCapturedCheeseThisPulse[helpers::kMaxCheeses];
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_
