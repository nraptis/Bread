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
  bool Generate();
  bool SimulationPathingLoop(int pActionBudget, int* pOutActions);
  bool SimulationMainLoop();
  void Simulation();
  void ShuffleStack();
  void SetInitialWalls();
  void Build();

  peanutbutter::fast_rand::FastRand mFastRand;
  ProbeConfig mProbeConfig;
  ProbeStats mProbeStats;
  int mGameIndex;
  const char* mGameName;
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZEDIRECTOR_HPP_
