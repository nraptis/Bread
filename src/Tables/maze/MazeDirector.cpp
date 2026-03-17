#include "MazeDirector.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include "MazeHelpers.hpp"

namespace peanutbutter::maze {

namespace {

using helpers::CheeseState;
using helpers::PowerUpType;
using helpers::RobotState;
using helpers::SharkState;

}  // namespace

MazeDirector::MazeDirector()
    : Maze(),
      mFastRand(),
      mProbeConfig{},
      mProbeStats{},
      mGameIndex(0),
      mGameName(GetProbeConfigNameByIndex(0)) {
  SetGame(0);
}

void MazeDirector::Seed(unsigned char* pPassword, int pPasswordLength) {
  mRuntimeStats = RuntimeStats{};
  mProbeStats = ProbeStats{};
  InitializeSeedBuffer(pPassword, pPasswordLength);
  mFastRand.Seed(pPassword, pPasswordLength);
  mFastRand.Mix(mSeedBuffer, static_cast<int>(mResultBufferLength));

  Simulation();
  FinalizeWalls();
}

void MazeDirector::SetGame(int pGameIndex) {
  mGameIndex = NormalizeGameIndex(pGameIndex);
  mProbeConfig = GetProbeConfigByIndex(mGameIndex);
  mGameName = GetProbeConfigNameByIndex(mGameIndex);
}

int MazeDirector::CurrentGame() const {
  return mGameIndex;
}

const char* MazeDirector::GetName() const {
  return mGameName;
}

void MazeDirector::SetProbeConfig(const ProbeConfig& pConfig) {
  mProbeConfig = pConfig;
  mGameIndex = -1;
  mGameName = "CustomProbe";
}

ProbeConfig MazeDirector::GetProbeConfig() const {
  return mProbeConfig;
}

MazeDirector::ProbeStats MazeDirector::GetProbeStats() const {
  return mProbeStats;
}

int MazeDirector::NextIndex(int pLimit) {
  if (pLimit <= 1) {
    return 0;
  }
  return static_cast<int>(mFastRand.GetInt() % static_cast<unsigned int>(pLimit));
}

int MazeDirector::ClampConfiguredCount(int pConfiguredCount, int pMaximum) const {
  if (pConfiguredCount < 0) {
    return 0;
  }
  return (pConfiguredCount <= pMaximum) ? pConfiguredCount : pMaximum;
}

GenerationMode MazeDirector::ResolveGenerationMode() const {
  if (mProbeConfig.mGenerationMode != GenerationMode::kRandom) {
    return mProbeConfig.mGenerationMode;
  }
  return static_cast<GenerationMode>(const_cast<MazeDirector*>(this)->NextIndex(3));
}

bool MazeDirector::Generate() {
  Reset();
  Build();
  FinalizeWalls();
  if (mWalkableListCount <= 0) {
    SetFlushAccountingMode(FlushAccountingMode::kStalled);
    Flush();
    SetFlushAccountingMode(FlushAccountingMode::kRegular);
    return false;
  }
  (void)PaintWalkableFromSeed();
  return true;
}

bool MazeDirector::SimulationPathingLoop(int pActionBudget, int* pOutActions) {
  std::array<RobotState, helpers::kMaxRobots> aRobots = {};
  std::array<CheeseState, helpers::kMaxCheeses> aCheeses = {};
  std::array<SharkState, helpers::kMaxSharks> aSharks = {};
  unsigned char aPowerUpType[kGridWidth][kGridHeight] = {};
  int aActions = 0;
  int aRobotWalks[helpers::kMaxRobots] = {};
  int aSharkMovesSinceKill[helpers::kMaxSharks] = {};

  if (pOutActions != nullptr) {
    *pOutActions = 0;
  }

  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  const int aCheeseCount = ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses);
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);

  auto IsOccupied = [&](int pX, int pY) -> bool {
    for (int aIndex = 0; aIndex < aRobotCount; ++aIndex) {
      if (aRobots[aIndex].mAlive && aRobots[aIndex].mX == pX && aRobots[aIndex].mY == pY) {
        return true;
      }
    }
    for (int aIndex = 0; aIndex < aCheeseCount; ++aIndex) {
      if (aCheeses[aIndex].mActive && aCheeses[aIndex].mX == pX && aCheeses[aIndex].mY == pY) {
        return true;
      }
    }
    for (int aIndex = 0; aIndex < aSharkCount; ++aIndex) {
      if (aSharks[aIndex].mActive && aSharks[aIndex].mX == pX && aSharks[aIndex].mY == pY) {
        return true;
      }
    }
    return aPowerUpType[pX][pY] != 0U;
  };

  auto GetRandomWalkableFn = [&](int& pX, int& pY) -> bool { return GetRandomWalkable(pX, pY); };
  auto RecordDeath = [&]() -> bool {
    ++mRuntimeStats.mDeaths;
    ++aActions;
    if (pOutActions != nullptr) {
      *pOutActions = aActions;
    }
    return aActions >= pActionBudget;
  };
  auto RecordVictory = [&]() -> bool {
    ++mRuntimeStats.mVictories;
    ++aActions;
    if (pOutActions != nullptr) {
      *pOutActions = aActions;
    }
    return aActions >= pActionBudget;
  };
  auto FinalizeProbeStats = [&]() {
    ++mProbeStats.mSimulationCount;
    for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
      ++mProbeStats.mRobotLifeCount;
      mProbeStats.mTotalRobotWalk += static_cast<std::uint64_t>(aRobotWalks[aRobotIndex]);
      if (mProbeStats.mRobotLifeCount == 1U ||
          static_cast<std::uint64_t>(aRobotWalks[aRobotIndex]) < mProbeStats.mShortestRobotWalk) {
        mProbeStats.mShortestRobotWalk = static_cast<std::uint64_t>(aRobotWalks[aRobotIndex]);
      }
      if (static_cast<std::uint64_t>(aRobotWalks[aRobotIndex]) > mProbeStats.mLongestRobotWalk) {
        mProbeStats.mLongestRobotWalk = static_cast<std::uint64_t>(aRobotWalks[aRobotIndex]);
      }
    }
    if (pOutActions != nullptr) {
      *pOutActions = aActions;
    }
  };
  auto ResetRobotState = [&](RobotState& pRobot, int pRobotIndex) {
    pRobot.mAlive = false;
    pRobot.mVictorious = false;
    pRobot.mTargetCheese = -1;
    pRobot.mInvincibleHops = 0;
    pRobot.mMagnetHops = 0;
    pRobot.mSuperSpeedHops = 0;
    aRobotWalks[pRobotIndex] = 0;
  };
  auto RespawnRobot = [&](RobotState& pRobot, int pRobotIndex) -> bool {
    ResetRobotState(pRobot, pRobotIndex);
    int aRespawnX = -1;
    int aRespawnY = -1;
    if (!helpers::PickUniqueWalkable(GetRandomWalkableFn, IsOccupied, &aRespawnX, &aRespawnY)) {
      return false;
    }
    pRobot.mX = aRespawnX;
    pRobot.mY = aRespawnY;
    pRobot.mAlive = true;
    return true;
  };
  auto RespawnShark = [&](int pSharkIndex) -> bool {
    if (pSharkIndex < 0 || pSharkIndex >= aSharkCount) {
      return false;
    }
    SharkState& aShark = aSharks[pSharkIndex];
    aShark.mActive = false;
    int aRespawnX = -1;
    int aRespawnY = -1;
    if (!helpers::PickUniqueWalkable(GetRandomWalkableFn, IsOccupied, &aRespawnX, &aRespawnY)) {
      return false;
    }
    aShark.mX = aRespawnX;
    aShark.mY = aRespawnY;
    aShark.mActive = true;
    aSharkMovesSinceKill[pSharkIndex] = 0;
    return true;
  };

  for (int aIndex = 0; aIndex < aRobotCount; ++aIndex) {
    if (!helpers::PickUniqueWalkable(GetRandomWalkableFn, IsOccupied, &aRobots[aIndex].mX, &aRobots[aIndex].mY)) {
      ++mRuntimeStats.mInconsistentStateA;
      FinalizeProbeStats();
      return false;
    }
    aRobots[aIndex].mAlive = true;
  }

  for (int aIndex = 0; aIndex < aCheeseCount; ++aIndex) {
    if (!helpers::PickUniqueWalkable(GetRandomWalkableFn, IsOccupied, &aCheeses[aIndex].mX, &aCheeses[aIndex].mY)) {
      ++mRuntimeStats.mInconsistentStateA;
      FinalizeProbeStats();
      return false;
    }
    aCheeses[aIndex].mActive = true;
  }

  for (int aIndex = 0; aIndex < aSharkCount; ++aIndex) {
    if (!helpers::PickUniqueWalkable(GetRandomWalkableFn, IsOccupied, &aSharks[aIndex].mX, &aSharks[aIndex].mY)) {
      ++mRuntimeStats.mInconsistentStateA;
      FinalizeProbeStats();
      return false;
    }
    aSharks[aIndex].mActive = true;
  }

  for (int aIndex = 0; aIndex < mWalkableListCount; ++aIndex) {
    const int aX = mWalkableListX[aIndex];
    const int aY = mWalkableListY[aIndex];
    if (IsOccupied(aX, aY) || NextIndex(helpers::kPowerUpSpawnDivisor) != 0) {
      continue;
    }
    aPowerUpType[aX][aY] = static_cast<unsigned char>(NextIndex(4) + 1);
  }

  auto ConsumeRobotSquare = [&](int pCenterX, int pCenterY) {
    for (int aYOffset = -1; aYOffset <= 1; ++aYOffset) {
      for (int aXOffset = -1; aXOffset <= 1; ++aXOffset) {
        const int aX = pCenterX + aXOffset;
        const int aY = pCenterY + aYOffset;
        if (IsWall(aX, aY)) {
          continue;
        }
        if (RepaintFromSeed(aX, aY)) {
          ++mRuntimeStats.mTilesPaintedValidScenario;
        } else {
          Flush(aX, aY);
        }
      }
    }
  };

  auto HasSharkAt = [&](int pX, int pY) -> bool {
    for (int aIndex = 0; aIndex < aSharkCount; ++aIndex) {
      if (aSharks[aIndex].mActive && aSharks[aIndex].mX == pX && aSharks[aIndex].mY == pY) {
        return true;
      }
    }
    return false;
  };
  auto SharkIndexAt = [&](int pX, int pY) -> int {
    for (int aIndex = 0; aIndex < aSharkCount; ++aIndex) {
      if (aSharks[aIndex].mActive && aSharks[aIndex].mX == pX && aSharks[aIndex].mY == pY) {
        return aIndex;
      }
    }
    return -1;
  };

  auto ClearCheeseTargets = [&](int pCheeseIndex, int pSkipRobotIndex) {
    for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
      if (aRobotIndex != pSkipRobotIndex && aRobots[aRobotIndex].mTargetCheese == pCheeseIndex) {
        aRobots[aRobotIndex].mTargetCheese = -1;
      }
    }
  };

  auto AssignTarget = [&](RobotState& pRobot) -> bool {
    if (!pRobot.mAlive || pRobot.mVictorious) {
      return false;
    }
    const int aCheeseIndex = helpers::ChooseActiveCheese([&](int pLimit) { return NextIndex(pLimit); }, aCheeses,
                                                         aCheeseCount);
    pRobot.mTargetCheese = aCheeseIndex;
    return aCheeseIndex >= 0;
  };
  auto RetargetRobots = [&](int pCollectedCheeseIndex) -> bool {
    for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
      RobotState& aRobot = aRobots[aRobotIndex];
      if (!aRobot.mAlive) {
        continue;
      }
      if (aRobot.mTargetCheese == pCollectedCheeseIndex ||
          aRobot.mTargetCheese < 0 ||
          aRobot.mTargetCheese >= aCheeseCount ||
          !aCheeses[aRobot.mTargetCheese].mActive) {
        if (!AssignTarget(aRobot)) {
          return false;
        }
      }
    }
    return true;
  };

  auto HandleRobotSharkCollision = [&](RobotState& pRobot, int pRobotIndex) -> bool {
    if (pRobot.mInvincibleHops > 0) {
      return false;
    }
    const int aSharkIndex = SharkIndexAt(pRobot.mX, pRobot.mY);
    if (aSharkIndex < 0) {
      return false;
    }

    ++mProbeStats.mSharkKillCount;
    mProbeStats.mTotalSharkMovesBeforeKill += static_cast<std::uint64_t>(aSharkMovesSinceKill[aSharkIndex]);
    if (!RespawnShark(aSharkIndex)) {
      return true;
    }

    ResetRobotState(pRobot, pRobotIndex);
    if (RecordDeath()) {
      return true;
    }
    return !RespawnRobot(pRobot, pRobotIndex);
  };

  auto ApplyTeleport = [&](RobotState& pRobot) {
    if (pRobot.mTargetCheese < 0 || pRobot.mTargetCheese >= aCheeseCount || !aCheeses[pRobot.mTargetCheese].mActive) {
      return;
    }
    const int aTargetX = aCheeses[pRobot.mTargetCheese].mX;
    const int aTargetY = aCheeses[pRobot.mTargetCheese].mY;
    int aTeleportX = -1;
    int aTeleportY = -1;
    auto IsTeleportBlocked = [&](int pX, int pY) -> bool { return HasSharkAt(pX, pY); };
    if (helpers::PickWalkableNearTarget(*this, [&](int pLimit) { return NextIndex(pLimit); }, aTargetX, aTargetY, 5,
                                        IsTeleportBlocked, &aTeleportX, &aTeleportY)) {
      pRobot.mX = aTeleportX;
      pRobot.mY = aTeleportY;
      ConsumeRobotSquare(pRobot.mX, pRobot.mY);
    }
  };

  auto ApplyNearbyPowerUps = [&](RobotState& pRobot) {
    const int aCollectRadius = (pRobot.mMagnetHops > 0) ? 2 : 1;
    for (int aY = pRobot.mY - aCollectRadius; aY <= pRobot.mY + aCollectRadius; ++aY) {
      for (int aX = pRobot.mX - aCollectRadius; aX <= pRobot.mX + aCollectRadius; ++aX) {
        if (aX < 0 || aX >= kGridWidth || aY < 0 || aY >= kGridHeight) {
          continue;
        }
        const PowerUpType aType = static_cast<PowerUpType>(aPowerUpType[aX][aY]);
        if (aType == PowerUpType::kNone) {
          continue;
        }
        aPowerUpType[aX][aY] = 0U;
        switch (aType) {
          case PowerUpType::kInvincible:
            pRobot.mInvincibleHops = helpers::RandomDurationFromPick(NextIndex(7));
            break;
          case PowerUpType::kMagnet:
            pRobot.mMagnetHops = helpers::RandomDurationFromPick(NextIndex(7));
            break;
          case PowerUpType::kSuperSpeed:
            pRobot.mSuperSpeedHops = helpers::RandomDurationFromPick(NextIndex(7));
            break;
          case PowerUpType::kTeleport:
            ApplyTeleport(pRobot);
            break;
          default:
            break;
        }
      }
    }
  };

  auto CollectCheese = [&](RobotState& pRobot, int pRobotIndex, bool* pRoundComplete) -> bool {
    if (pRoundComplete != nullptr) {
      *pRoundComplete = false;
    }
    const int aCollectRadius = (pRobot.mMagnetHops > 0) ? 2 : 1;
    for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
      if (!aCheeses[aCheeseIndex].mActive) {
        continue;
      }
      if (std::abs(aCheeses[aCheeseIndex].mX - pRobot.mX) > aCollectRadius ||
          std::abs(aCheeses[aCheeseIndex].mY - pRobot.mY) > aCollectRadius) {
        continue;
      }
      aCheeses[aCheeseIndex].mActive = false;
      pRobot.mTargetCheese = -1;
      ClearCheeseTargets(aCheeseIndex, pRobotIndex);
      if (!RespawnRobot(pRobot, pRobotIndex) || !RetargetRobots(aCheeseIndex)) {
        if (pRoundComplete != nullptr) {
          *pRoundComplete = true;
        }
      }
      (void)RecordVictory();
      if (pRoundComplete != nullptr && aActions >= pActionBudget) {
        *pRoundComplete = true;
      }
      return true;
    }
    return false;
  };

  for (int aTick = 0; aTick < helpers::kMaxTrialTicks; ++aTick) {
    if (!helpers::AnyActiveCheese(aCheeses, aCheeseCount) || !helpers::AnyPathingRobot(aRobots, aRobotCount)) {
      FinalizeProbeStats();
      return true;
    }

    bool aTickDidProgress = false;

    for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
      RobotState& aRobot = aRobots[aRobotIndex];
      if (!aRobot.mAlive || aRobot.mVictorious) {
        continue;
      }
      if (aRobot.mTargetCheese < 0 || aRobot.mTargetCheese >= aCheeseCount || !aCheeses[aRobot.mTargetCheese].mActive) {
        if (!AssignTarget(aRobot)) {
          FinalizeProbeStats();
          return true;
        }
      }

      const int aTargetX = aCheeses[aRobot.mTargetCheese].mX;
      const int aTargetY = aCheeses[aRobot.mTargetCheese].mY;
      if (!FindPath(aRobot.mX, aRobot.mY, aTargetX, aTargetY)) {
        ++mRuntimeStats.mInconsistentStateA;
        aRobot.mAlive = false;
        if (RecordDeath()) {
          FinalizeProbeStats();
          return true;
        }
        continue;
      }

      const int aHopBudget = (aRobot.mSuperSpeedHops > 0) ? 2 : 1;
      int aStepsTaken = 0;
      for (int aHop = 0; aHop < aHopBudget && aRobot.mAlive && !aRobot.mVictorious; ++aHop) {
        const int aPathIndex = aHop + 1;
        if (aPathIndex >= PathLength()) {
          break;
        }
        if (!PathNode(aPathIndex, &aRobot.mX, &aRobot.mY)) {
          ++mRuntimeStats.mInconsistentStateA;
          aRobot.mAlive = false;
          if (RecordDeath()) {
            FinalizeProbeStats();
            return true;
          }
          break;
        }

        ConsumeRobotSquare(aRobot.mX, aRobot.mY);
        ++aStepsTaken;
        ++aRobotWalks[aRobotIndex];
        aTickDidProgress = true;

        if (HasSharkAt(aRobot.mX, aRobot.mY) && aRobot.mInvincibleHops <= 0) {
          if (HandleRobotSharkCollision(aRobot, aRobotIndex)) {
            FinalizeProbeStats();
            return true;
          }
          break;
        }

        ApplyNearbyPowerUps(aRobot);
        if (HasSharkAt(aRobot.mX, aRobot.mY) && aRobot.mInvincibleHops <= 0) {
          if (HandleRobotSharkCollision(aRobot, aRobotIndex)) {
            FinalizeProbeStats();
            return true;
          }
          break;
        }
        bool aRoundComplete = false;
        if (CollectCheese(aRobot, aRobotIndex, &aRoundComplete)) {
          if (aRoundComplete) {
            FinalizeProbeStats();
            return true;
          }
          break;
        }
      }

      aRobot.mInvincibleHops = std::max(0, aRobot.mInvincibleHops - aStepsTaken);
      aRobot.mMagnetHops = std::max(0, aRobot.mMagnetHops - aStepsTaken);
      aRobot.mSuperSpeedHops = std::max(0, aRobot.mSuperSpeedHops - aStepsTaken);
    }

    for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
      SharkState& aShark = aSharks[aSharkIndex];
      if (!aShark.mActive) {
        continue;
      }

      static constexpr int kDirX[4] = {-1, 1, 0, 0};
      static constexpr int kDirY[4] = {0, 0, -1, 1};
      int aDirOrder[4] = {0, 1, 2, 3};
      for (int aIndex = 3; aIndex > 0; --aIndex) {
        const int aSwapIndex = NextIndex(aIndex + 1);
        std::swap(aDirOrder[aIndex], aDirOrder[aSwapIndex]);
      }

      for (int aOrderIndex = 0; aOrderIndex < 4; ++aOrderIndex) {
        const int aDir = aDirOrder[aOrderIndex];
        const int aNextX = aShark.mX + kDirX[aDir];
        const int aNextY = aShark.mY + kDirY[aDir];
        if (IsWall(aNextX, aNextY)) {
          continue;
        }

        bool aBlocked = false;
        for (int aOtherSharkIndex = 0; aOtherSharkIndex < aSharkCount; ++aOtherSharkIndex) {
          if (aOtherSharkIndex == aSharkIndex || !aSharks[aOtherSharkIndex].mActive) {
            continue;
          }
          if (aSharks[aOtherSharkIndex].mX == aNextX && aSharks[aOtherSharkIndex].mY == aNextY) {
            aBlocked = true;
            break;
          }
        }
        if (aBlocked) {
          continue;
        }

        aShark.mX = aNextX;
        aShark.mY = aNextY;
        ++aSharkMovesSinceKill[aSharkIndex];
        aTickDidProgress = true;
        break;
      }
    }

    for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
      RobotState& aRobot = aRobots[aRobotIndex];
      if (!aRobot.mAlive || aRobot.mVictorious || aRobot.mInvincibleHops > 0) {
        continue;
      }
      if (HasSharkAt(aRobot.mX, aRobot.mY)) {
        if (HandleRobotSharkCollision(aRobot, aRobotIndex)) {
          FinalizeProbeStats();
          return true;
        }
      }
    }

    if (!aTickDidProgress) {
      FinalizeProbeStats();
      return true;
    }
  }

  FinalizeProbeStats();
  return true;
}

bool MazeDirector::SimulationMainLoop() {
  if (!Generate()) {
    return false;
  }

  int aActionsThisMaze = 0;
  while (aActionsThisMaze < kMazeRespawnActionThreshold && mResultBufferWriteProgress < mResultBufferLength) {
    int aActionsThisTrial = 0;
    if (!SimulationPathingLoop(kMazeRespawnActionThreshold - aActionsThisMaze, &aActionsThisTrial)) {
      return false;
    }
    aActionsThisMaze += aActionsThisTrial;
  }

  return true;
}

void MazeDirector::Simulation() {
  int aStallCount = 0;
  while (mResultBufferWriteProgress < mResultBufferLength && aStallCount < kSimulationStallThreshold) {
    const std::uint64_t aWriteProgressBefore = mResultBufferWriteProgress;
    if (!SimulationMainLoop()) {
      break;
    }

    if (mResultBufferWriteProgress != aWriteProgressBefore) {
      aStallCount = 0;
      continue;
    }

    ++mRuntimeStats.mSimulationStallCataclysmic;
    ++aStallCount;
  }

  if (mResultBufferWriteProgress < mResultBufferLength) {
    ++mRuntimeStats.mSimulationStallApocalypse;
    ApocalypseScenario();
  }
}

void MazeDirector::ShuffleStack() {
  for (int aIndex = mStackCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = NextIndex(aIndex + 1);
    std::swap(mStackX[aIndex], mStackX[aSwapIndex]);
    std::swap(mStackY[aIndex], mStackY[aSwapIndex]);
  }
}

void MazeDirector::SetInitialWalls() {
  ClearWalls();
  const int aWalls = (kGridSize >> 2);
  for (int aIndex = 0; aIndex < aWalls && aIndex < mStackCount; ++aIndex) {
    SetWall(mStackX[aIndex], mStackY[aIndex], true);
  }
}

void MazeDirector::Build() {
  FillStackAllCoords();
  ShuffleStack();

  switch (ResolveGenerationMode()) {
    case GenerationMode::kCustom:
      SetInitialWalls();
      EnsureSingleConnectedOpenGroup();
      break;
    case GenerationMode::kPrim:
      GeneratePrims();
      break;
    case GenerationMode::kKruskal:
    default:
      ExecuteKruskals();
      break;
  }

  FinalizeWalls();
  if (mWalkableListCount <= 0) {
    ClearWalls();
    FinalizeWalls();
  }
}

}  // namespace peanutbutter::maze
