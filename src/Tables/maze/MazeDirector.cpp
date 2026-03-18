#include "MazeDirector.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

#include "MazeSpecialEvents.hpp"

namespace peanutbutter::maze {

namespace {

using helpers::MazeCheese;
using helpers::MazeDolphin;
using helpers::MazeRobot;
using helpers::MazeShark;
using helpers::PowerUpType;

constexpr int kMazeDolphinCount = helpers::kMaxDolphins;
constexpr int kMazeRespawnVictoryMinimum = 8;
constexpr int kMazeRespawnVictoryMaximum = 16;
constexpr int kMainLoopConsecutiveFailureThreshold = 4;

}  // namespace

MazeDirector::MazeDirector()
    : Maze(),
      mFastRand(),
      mProbeConfig{},
      mProbeStats{},
      mGameIndex(0),
      mGameName(GetProbeConfigNameByIndex(0)),
      mIsShark{},
      mIsDolphin{},
      mPowerUpType{},
      mRobotWalks{},
      mSharkMovesSinceKill{},
      mSharkMoveCandidateX{},
      mSharkMoveCandidateY{},
      mSharkMoveCandidateCount(0),
      mMainLoopIterationVictoryCount(0),
      mRobotLifeActive{},
      mCapturedCheeseThisPulse{} {
  SetGame(0);
}

void MazeDirector::Seed(unsigned char* pPassword, int pPasswordLength) {
  mRuntimeStats = RuntimeStats{};
  mProbeStats = ProbeStats{};
  InitializeSeedBuffer(pPassword, pPasswordLength);
  mFastRand.Seed(mSeedBuffer, pPasswordLength);
  ShuffleSeedBuffer(&mFastRand);

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

void MazeDirector::ResetPulseState() {
  for (MazeRobot& aRobot : mRobotStorage) {
    aRobot.Reset();
  }
  for (MazeCheese& aCheese : mCheeseStorage) {
    aCheese.Reset();
  }
  for (MazeShark& aShark : mSharkStorage) {
    aShark.Reset();
  }
  for (MazeDolphin& aDolphin : mDolphinStorage) {
    aDolphin.Reset();
  }
  ResetCharacterLists(ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots),
                      ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses),
                      ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks), kMazeDolphinCount);
  std::memset(mIsShark, 0, sizeof(mIsShark));
  std::memset(mIsDolphin, 0, sizeof(mIsDolphin));
  std::memset(mPowerUpType, 0, sizeof(mPowerUpType));
  std::memset(mRobotWalks, 0, sizeof(mRobotWalks));
  std::memset(mSharkMovesSinceKill, 0, sizeof(mSharkMovesSinceKill));
  std::memset(mSharkMoveCandidateX, 0, sizeof(mSharkMoveCandidateX));
  std::memset(mSharkMoveCandidateY, 0, sizeof(mSharkMoveCandidateY));
  mSharkMoveCandidateCount = 0;
  std::memset(mRobotLifeActive, 0, sizeof(mRobotLifeActive));
  ResetPulseOutcomeFlags();
}

void MazeDirector::ResetPulseOutcomeFlags() {
  std::memset(mCapturedCheeseThisPulse, 0, sizeof(mCapturedCheeseThisPulse));
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    if (mRobotList[aRobotIndex] == nullptr) {
      continue;
    }
    mRobotList[aRobotIndex]->mIsVictorious = false;
  }
}

bool MazeDirector::IsTileOccupied(int pX,
                                  int pY,
                                  int pIgnoreRobotIndex,
                                  int pIgnoreCheeseIndex,
                                  int pIgnoreSharkIndex,
                                  int pIgnoreDolphinIndex) const {
  if (!InBounds(pX, pY)) {
    return true;
  }

  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    const MazeRobot* aRobot = mRobotList[aRobotIndex];
    if (aRobotIndex == pIgnoreRobotIndex || aRobot == nullptr || aRobot->mDeadFlag) {
      continue;
    }
    if (aRobot->mX == pX && aRobot->mY == pY) {
      return true;
    }
  }

  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    const MazeCheese* aCheese = mCheeseList[aCheeseIndex];
    if (aCheeseIndex == pIgnoreCheeseIndex) {
      continue;
    }
    if (aCheese != nullptr && aCheese->mX == pX && aCheese->mY == pY) {
      return true;
    }
  }

  for (int aSharkIndex = 0; aSharkIndex < mSharkListCount; ++aSharkIndex) {
    const MazeShark* aShark = mSharkList[aSharkIndex];
    if (aSharkIndex == pIgnoreSharkIndex || aShark == nullptr || aShark->mDeadFlag) {
      continue;
    }
    if (aShark->mX == pX && aShark->mY == pY) {
      return true;
    }
  }

  for (int aDolphinIndex = 0; aDolphinIndex < mDolphinListCount; ++aDolphinIndex) {
    const MazeDolphin* aDolphin = mDolphinList[aDolphinIndex];
    if (aDolphinIndex == pIgnoreDolphinIndex || aDolphin == nullptr || aDolphin->mDeadFlag) {
      continue;
    }
    if (aDolphin->mX == pX && aDolphin->mY == pY) {
      return true;
    }
  }

  return mPowerUpType[pX][pY] != 0U;
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
  return InitializePulseState();
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

bool MazeDirector::SimulationMainLoop() {
  if (!Generate()) {
    return false;
  }

  int aConsecutiveFailureCount = 0;
  while (aConsecutiveFailureCount < kMainLoopConsecutiveFailureThreshold &&
         mResultBufferWriteProgress < mResultBufferLength) {
    const std::uint64_t aWriteProgressBeforeRound = mResultBufferWriteProgress;
    const int aVictoryThreshold =
        kMazeRespawnVictoryMinimum + NextIndex(kMazeRespawnVictoryMaximum - kMazeRespawnVictoryMinimum + 1);
    mMainLoopIterationVictoryCount = 0;

    while (mMainLoopIterationVictoryCount < aVictoryThreshold && mResultBufferWriteProgress < mResultBufferLength) {
      if (!PathingLoopPulse()) {
        ++mRuntimeStats.mInconsistentStateG;
        FinalizePulseStats();
        return true;
      }
    }

    if (mResultBufferWriteProgress >= mResultBufferLength) {
      break;
    }

    const bool aMadeByteProgress = (mResultBufferWriteProgress != aWriteProgressBeforeRound);
    if (mMainLoopIterationVictoryCount >= aVictoryThreshold || aMadeByteProgress) {
      aConsecutiveFailureCount = 0;
    } else {
      ++aConsecutiveFailureCount;
    }
  }

  if (aConsecutiveFailureCount >= kMainLoopConsecutiveFailureThreshold &&
      mResultBufferWriteProgress < mResultBufferLength) {
    ++mRuntimeStats.mInconsistentStateF;
    return false;
  }

  FinalizePulseStats();
  return true;
}

bool MazeDirector::PathingLoopPulse() {
  ResetPulseOutcomeFlags();
  HandleSpecialEvents();
  if (!MoveSharks()) {
    return false;
  }
  if (!MoveDolphins()) {
    return false;
  }
  if (!MoveRobots()) {
    return false;
  }
  MarkRobotSharkCollisions();
  MarkDolphinSharkCollisions();
  if (!ResolveCapturedCheeses()) {
    return false;
  }
  if (!ResolveDeadEntities()) {
    return false;
  }
  return ResolveRobotRepaths();
}

void MazeDirector::FinalizeRobotLife(int pRobotIndex) {
  if (pRobotIndex < 0 || pRobotIndex >= helpers::kMaxRobots || !mRobotLifeActive[pRobotIndex]) {
    return;
  }

  ++mProbeStats.mRobotLifeCount;
  mProbeStats.mTotalRobotWalk += static_cast<std::uint64_t>(mRobotWalks[pRobotIndex]);
  if (mProbeStats.mRobotLifeCount == 1U ||
      static_cast<std::uint64_t>(mRobotWalks[pRobotIndex]) < mProbeStats.mShortestRobotWalk) {
    mProbeStats.mShortestRobotWalk = static_cast<std::uint64_t>(mRobotWalks[pRobotIndex]);
  }
  if (static_cast<std::uint64_t>(mRobotWalks[pRobotIndex]) > mProbeStats.mLongestRobotWalk) {
    mProbeStats.mLongestRobotWalk = static_cast<std::uint64_t>(mRobotWalks[pRobotIndex]);
  }

  mRobotLifeActive[pRobotIndex] = false;
  mRobotWalks[pRobotIndex] = 0;
}

void MazeDirector::FinalizePulseStats() {
  ++mProbeStats.mSimulationCount;
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    FinalizeRobotLife(aRobotIndex);
  }
}

bool MazeDirector::RespawnRobot(int pRobotIndex, bool pMarkRecentlyRevived) {
  if (pRobotIndex < 0 || pRobotIndex >= helpers::kMaxRobots) {
    return false;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, pRobotIndex, -1, -1); }, &aRespawnX, &aRespawnY)) {
    ++mRuntimeStats.mInconsistentStateI;
    return false;
  }

  if (pRobotIndex >= mRobotListCount || mRobotList[pRobotIndex] == nullptr) {
    return false;
  }
  MazeRobot& aRobot = *mRobotList[pRobotIndex];
  aRobot.Reset();
  aRobot.mX = aRespawnX;
  aRobot.mY = aRespawnY;
  aRobot.mDeadFlag = false;
  aRobot.mDidRecentlyRepath = pMarkRecentlyRevived;
  mRobotWalks[pRobotIndex] = 0;
  mRobotLifeActive[pRobotIndex] = true;
  return true;
}

bool MazeDirector::RespawnCheese(int pCheeseIndex) {
  if (pCheeseIndex < 0 || pCheeseIndex >= helpers::kMaxCheeses) {
    return false;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, -1, pCheeseIndex, -1); }, &aRespawnX, &aRespawnY)) {
    ++mRuntimeStats.mInconsistentStateI;
    return false;
  }

  if (pCheeseIndex >= mCheeseListCount || mCheeseList[pCheeseIndex] == nullptr) {
    return false;
  }
  MazeCheese& aCheese = *mCheeseList[pCheeseIndex];
  aCheese.Reset();
  aCheese.mX = aRespawnX;
  aCheese.mY = aRespawnY;
  return true;
}

bool MazeDirector::RespawnShark(int pSharkIndex, bool pMarkRecentlyRevived) {
  if (pSharkIndex < 0 || pSharkIndex >= helpers::kMaxSharks) {
    return false;
  }

  if (pSharkIndex >= mSharkListCount || mSharkList[pSharkIndex] == nullptr) {
    return false;
  }
  MazeShark& aShark = *mSharkList[pSharkIndex];
  if (!aShark.mDeadFlag && InBounds(aShark.mX, aShark.mY)) {
    mIsShark[aShark.mX][aShark.mY] = 0U;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, -1, -1, pSharkIndex); }, &aRespawnX, &aRespawnY)) {
    ++mRuntimeStats.mInconsistentStateI;
    return false;
  }

  aShark.mX = aRespawnX;
  aShark.mY = aRespawnY;
  aShark.mDeadFlag = false;
  aShark.mDidRecentlyRevive = pMarkRecentlyRevived;
  mIsShark[aShark.mX][aShark.mY] = 1U;
  mSharkMovesSinceKill[pSharkIndex] = 0;
  return true;
}

bool MazeDirector::RespawnDolphin(int pDolphinIndex, bool pMarkRecentlyRevived) {
  if (pDolphinIndex < 0 || pDolphinIndex >= helpers::kMaxDolphins) {
    return false;
  }

  if (pDolphinIndex >= mDolphinListCount || mDolphinList[pDolphinIndex] == nullptr) {
    return false;
  }
  MazeDolphin& aDolphin = *mDolphinList[pDolphinIndex];
  if (!aDolphin.mDeadFlag && InBounds(aDolphin.mX, aDolphin.mY)) {
    mIsDolphin[aDolphin.mX][aDolphin.mY] = 0U;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, -1, -1, -1, pDolphinIndex); }, &aRespawnX,
          &aRespawnY)) {
    ++mRuntimeStats.mInconsistentStateI;
    return false;
  }

  aDolphin.mX = aRespawnX;
  aDolphin.mY = aRespawnY;
  aDolphin.mDeadFlag = false;
  aDolphin.mDidRecentlyRevive = pMarkRecentlyRevived;
  mIsDolphin[aDolphin.mX][aDolphin.mY] = 1U;
  (void)RepaintOrFlushTile(aDolphin.mX, aDolphin.mY);
  return true;
}

bool MazeDirector::InitializePulseState() {
  ResetPulseState();

  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    if (!RespawnCheese(aCheeseIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    if (!RespawnRobot(aRobotIndex, false)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aSharkIndex = 0; aSharkIndex < mSharkListCount; ++aSharkIndex) {
    if (!RespawnShark(aSharkIndex, false)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aDolphinIndex = 0; aDolphinIndex < mDolphinListCount; ++aDolphinIndex) {
    if (!RespawnDolphin(aDolphinIndex, false)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    MazeRobot& aRobot = *mRobotList[aRobotIndex];
    aRobot.mNeedsRepath = true;
  }

  for (int aIndex = 0; aIndex < mWalkableListCount; ++aIndex) {
    const int aX = mWalkableListX[aIndex];
    const int aY = mWalkableListY[aIndex];
    if (IsTileOccupied(aX, aY) || NextIndex(helpers::kPowerUpSpawnDivisor) != 0) {
      continue;
    }
    mPowerUpType[aX][aY] = static_cast<unsigned char>(NextIndex(4) + 1);
  }

  return ResolveRobotRepaths();
}

int MazeDirector::FindSharkAt(int pX, int pY) const {
  if (!InBounds(pX, pY) || mIsShark[pX][pY] == 0U) {
    return -1;
  }
  for (int aSharkIndex = 0; aSharkIndex < mSharkListCount; ++aSharkIndex) {
    const MazeShark* aShark = mSharkList[aSharkIndex];
    if (aShark == nullptr || aShark->mDeadFlag) {
      continue;
    }
    if (aShark->mX == pX && aShark->mY == pY) {
      return aSharkIndex;
    }
  }
  return -1;
}

int MazeDirector::FindCheeseAt(int pX, int pY) const {
  if (!InBounds(pX, pY)) {
    return -1;
  }
  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    const MazeCheese* aCheese = mCheeseList[aCheeseIndex];
    if (aCheese == nullptr) {
      continue;
    }
    if (aCheese->mX == pX && aCheese->mY == pY) {
      return aCheeseIndex;
    }
  }
  return -1;
}

void MazeDirector::MoveShark(MazeShark& pShark, int pSharkIndex) {
  if (pSharkIndex < 0 || pSharkIndex >= helpers::kMaxSharks || pShark.mDeadFlag || !InBounds(pShark.mX, pShark.mY)) {
    return;
  }

  mSharkMoveCandidateCount = 0;

  int aCheckX = pShark.mX - 1;
  int aCheckY = pShark.mY;
  if (IsWalkable(aCheckX, aCheckY) && mIsShark[aCheckX][aCheckY] == 0U) {
    mSharkMoveCandidateX[mSharkMoveCandidateCount] = aCheckX;
    mSharkMoveCandidateY[mSharkMoveCandidateCount] = aCheckY;
    ++mSharkMoveCandidateCount;
  }

  aCheckX = pShark.mX + 1;
  aCheckY = pShark.mY;
  if (IsWalkable(aCheckX, aCheckY) && mIsShark[aCheckX][aCheckY] == 0U) {
    mSharkMoveCandidateX[mSharkMoveCandidateCount] = aCheckX;
    mSharkMoveCandidateY[mSharkMoveCandidateCount] = aCheckY;
    ++mSharkMoveCandidateCount;
  }

  aCheckX = pShark.mX;
  aCheckY = pShark.mY - 1;
  if (IsWalkable(aCheckX, aCheckY) && mIsShark[aCheckX][aCheckY] == 0U) {
    mSharkMoveCandidateX[mSharkMoveCandidateCount] = aCheckX;
    mSharkMoveCandidateY[mSharkMoveCandidateCount] = aCheckY;
    ++mSharkMoveCandidateCount;
  }

  aCheckX = pShark.mX;
  aCheckY = pShark.mY + 1;
  if (IsWalkable(aCheckX, aCheckY) && mIsShark[aCheckX][aCheckY] == 0U) {
    mSharkMoveCandidateX[mSharkMoveCandidateCount] = aCheckX;
    mSharkMoveCandidateY[mSharkMoveCandidateCount] = aCheckY;
    ++mSharkMoveCandidateCount;
  }

  if (mSharkMoveCandidateCount > 0) {
    mIsShark[pShark.mX][pShark.mY] = 0U;
    const int aPickIndex = NextIndex(mSharkMoveCandidateCount);
    pShark.mX = mSharkMoveCandidateX[aPickIndex];
    pShark.mY = mSharkMoveCandidateY[aPickIndex];
    mIsShark[pShark.mX][pShark.mY] = 1U;
    ++mSharkMovesSinceKill[pSharkIndex];
  }
}

void MazeDirector::MoveDolphin(MazeDolphin& pDolphin, int pDolphinIndex) {
  if (pDolphinIndex < 0 || pDolphinIndex >= helpers::kMaxDolphins || pDolphin.mDeadFlag ||
      !InBounds(pDolphin.mX, pDolphin.mY)) {
    return;
  }

  int aCandidateX[4] = {};
  int aCandidateY[4] = {};
  int aCandidateScore[4] = {};
  int aCandidateCount = 0;

  const int aStepX[4] = {-1, 1, 0, 0};
  const int aStepY[4] = {0, 0, -1, 1};
  for (int aStepIndex = 0; aStepIndex < 4; ++aStepIndex) {
    const int aCheckX = pDolphin.mX + aStepX[aStepIndex];
    const int aCheckY = pDolphin.mY + aStepY[aStepIndex];
    if (!IsWalkable(aCheckX, aCheckY) || mIsDolphin[aCheckX][aCheckY] != 0U) {
      continue;
    }

    bool aHasRobot = false;
    for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
      const MazeRobot* aRobot = mRobotList[aRobotIndex];
      if (aRobot != nullptr && !aRobot->mDeadFlag && aRobot->mX == aCheckX && aRobot->mY == aCheckY) {
        aHasRobot = true;
        break;
      }
    }
    if (aHasRobot) {
      continue;
    }

    int aBestCheeseDistance = kGridSize;
    for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
      const MazeCheese* aCheese = mCheeseList[aCheeseIndex];
      if (aCheese == nullptr) {
        continue;
      }
      const int aDistance = AbsInt(aCheese->mX - aCheckX) + AbsInt(aCheese->mY - aCheckY);
      if (aDistance < aBestCheeseDistance) {
        aBestCheeseDistance = aDistance;
      }
    }

    int aScore = -aBestCheeseDistance;
    if (FindCheeseAt(aCheckX, aCheckY) >= 0) {
      aScore += 1000;
    }
    if (FindSharkAt(aCheckX, aCheckY) >= 0) {
      aScore += 500;
    }

    aCandidateX[aCandidateCount] = aCheckX;
    aCandidateY[aCandidateCount] = aCheckY;
    aCandidateScore[aCandidateCount] = aScore;
    ++aCandidateCount;
  }

  if (aCandidateCount <= 0) {
    return;
  }

  int aBestScore = aCandidateScore[0];
  for (int aIndex = 1; aIndex < aCandidateCount; ++aIndex) {
    if (aCandidateScore[aIndex] > aBestScore) {
      aBestScore = aCandidateScore[aIndex];
    }
  }

  int aBestCandidateIndex[4] = {};
  int aBestCandidateCount = 0;
  for (int aIndex = 0; aIndex < aCandidateCount; ++aIndex) {
    if (aCandidateScore[aIndex] != aBestScore) {
      continue;
    }
    aBestCandidateIndex[aBestCandidateCount] = aIndex;
    ++aBestCandidateCount;
  }

  mIsDolphin[pDolphin.mX][pDolphin.mY] = 0U;
  const int aPickIndex = aBestCandidateIndex[NextIndex(aBestCandidateCount)];
  pDolphin.mX = aCandidateX[aPickIndex];
  pDolphin.mY = aCandidateY[aPickIndex];
  mIsDolphin[pDolphin.mX][pDolphin.mY] = 1U;
}

MazeCheese* MazeDirector::PickRandomCheese(int pIgnoreCheeseIndex) {
  int aCandidateIndex[helpers::kMaxCheeses] = {};
  int aCandidateCount = 0;
  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    if (aCheeseIndex == pIgnoreCheeseIndex) {
      continue;
    }
    aCandidateIndex[aCandidateCount] = aCheeseIndex;
    ++aCandidateCount;
  }
  if (aCandidateCount <= 0) {
    return nullptr;
  }
  return mCheeseList[aCandidateIndex[NextIndex(aCandidateCount)]];
}

void MazeDirector::ClearRobotTarget(MazeRobot* pRobot) {
  if (pRobot == nullptr) {
    return;
  }

  pRobot->mCheese = nullptr;
  (void)pRobot->AssignPathAndStartWalk(nullptr, nullptr, 0);
}

bool MazeDirector::StoreRobotPath(MazeRobot* pRobot, MazeCheese* pCheese) {
  if (pRobot == nullptr || pCheese == nullptr || pRobot->mDeadFlag) {
    return false;
  }

  const bool aHasPath = FindPath(pRobot->mX, pRobot->mY, pCheese->mX, pCheese->mY);
  const int aPathLength = PathLength();
  if (!aHasPath || aPathLength <= 0) {
    ++mRuntimeStats.mInconsistentStateE;
    return false;
  }

  int aPathX[Maze::kGridSize] = {};
  int aPathY[Maze::kGridSize] = {};
  for (int aPathIndex = 0; aPathIndex < aPathLength; ++aPathIndex) {
    if (!PathNode(aPathIndex, &aPathX[aPathIndex], &aPathY[aPathIndex])) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }
  }
  ClearRobotTarget(pRobot);
  if (!pRobot->AssignPathAndStartWalk(aPathX, aPathY, aPathLength)) {
    ++mRuntimeStats.mInconsistentStateE;
    return false;
  }

  pRobot->mCheese = pCheese;
  pRobot->mNeedsRepath = false;
  return true;
}

bool MazeDirector::AssignPathableCheese(MazeRobot* pRobot, int pIgnoreCheeseIndex) {
  if (pRobot == nullptr || pRobot->mDeadFlag || pRobot->mIsVictorious) {
    return false;
  }

  MazeCheese* aCheese = PickRandomCheese(pIgnoreCheeseIndex);
  if (aCheese == nullptr) {
    ClearRobotTarget(pRobot);
    return false;
  }
  return StoreRobotPath(pRobot, aCheese);
}

bool MazeDirector::RepaintOrFlushTile(int pX, int pY) {
  if (IsWall(pX, pY)) {
    return false;
  }

  const bool aHadByte = (mByte[pX][pY] >= 0);
  if (RepaintFromSeed(pX, pY)) {
    ++mRuntimeStats.mTilesPaintedValidScenario;
    return true;
  }
  if (aHadByte) {
    Flush(pX, pY);
    return true;
  }
  return false;
}

void MazeDirector::RepaintRobotSquare(int pCenterX, int pCenterY) {
  if (!InBounds(pCenterX, pCenterY)) {
    return;
  }

  for (int aYOffset = -1; aYOffset <= 1; ++aYOffset) {
    for (int aXOffset = -1; aXOffset <= 1; ++aXOffset) {
      (void)RepaintOrFlushTile(pCenterX + aXOffset, pCenterY + aYOffset);
    }
  }
}

void MazeDirector::ApplyRobotPowerUp(int pRobotIndex) {
  if (pRobotIndex < 0 || pRobotIndex >= mRobotListCount || mRobotList[pRobotIndex] == nullptr) {
    return;
  }

  MazeRobot& aRobot = *mRobotList[pRobotIndex];
  if (aRobot.mDeadFlag || !InBounds(aRobot.mX, aRobot.mY)) {
    return;
  }

  for (int aY = aRobot.mY - 1; aY <= aRobot.mY + 1; ++aY) {
    for (int aX = aRobot.mX - 1; aX <= aRobot.mX + 1; ++aX) {
      if (!InBounds(aX, aY)) {
        continue;
      }
      const PowerUpType aType = static_cast<PowerUpType>(mPowerUpType[aX][aY]);
      if (aType == PowerUpType::kNone) {
        continue;
      }
      mPowerUpType[aX][aY] = 0U;
      aRobot.mPowerUp = aType;
      aRobot.mPowerUpTick = helpers::RandomDurationFromPick(NextIndex(7));
    }
  }
}

bool MazeDirector::MarkRobotVictory(int pRobotIndex, bool* pOutFailure) {
  if (pOutFailure != nullptr) {
    *pOutFailure = false;
  }
  if (pRobotIndex < 0 || pRobotIndex >= mRobotListCount || mRobotList[pRobotIndex] == nullptr) {
    return false;
  }

  MazeRobot& aRobot = *mRobotList[pRobotIndex];
  if (aRobot.mDeadFlag || aRobot.mIsVictorious) {
    return false;
  }

  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    MazeCheese& aCheese = *mCheeseList[aCheeseIndex];
    if (AbsInt(aCheese.mX - aRobot.mX) > 1 || AbsInt(aCheese.mY - aRobot.mY) > 1) {
      continue;
    }

    mCapturedCheeseThisPulse[aCheeseIndex] = true;
    aRobot.mIsVictorious = true;
    aRobot.mNeedsRepath = false;
    ++mRuntimeStats.mVictories;
    ++mMainLoopIterationVictoryCount;
    return true;
  }

  return false;
}

bool MazeDirector::MarkDolphinCheeseSteal(int pDolphinIndex) {
  if (pDolphinIndex < 0 || pDolphinIndex >= mDolphinListCount || mDolphinList[pDolphinIndex] == nullptr) {
    return false;
  }

  MazeDolphin& aDolphin = *mDolphinList[pDolphinIndex];
  if (aDolphin.mDeadFlag) {
    return true;
  }

  const int aCheeseIndex = FindCheeseAt(aDolphin.mX, aDolphin.mY);
  if (aCheeseIndex < 0 || aCheeseIndex >= mCheeseListCount || mCheeseList[aCheeseIndex] == nullptr) {
    return true;
  }

  if (mCapturedCheeseThisPulse[aCheeseIndex]) {
    return true;
  }

  mCapturedCheeseThisPulse[aCheeseIndex] = true;
  ++mRuntimeStats.mDolphinCheeseTriages;
  RepaintRobotSquare(aDolphin.mX, aDolphin.mY);
  return true;
}

void MazeDirector::MarkDolphinAndSharkDead(int pDolphinIndex, int pSharkIndex) {
  if (pDolphinIndex < 0 || pDolphinIndex >= mDolphinListCount || mDolphinList[pDolphinIndex] == nullptr ||
      pSharkIndex < 0 || pSharkIndex >= mSharkListCount || mSharkList[pSharkIndex] == nullptr) {
    return;
  }

  MazeDolphin& aDolphin = *mDolphinList[pDolphinIndex];
  MazeShark& aShark = *mSharkList[pSharkIndex];
  if (aDolphin.mDeadFlag || aShark.mDeadFlag) {
    return;
  }

  ++mRuntimeStats.mDolphinSharkCollisions;
  if (NextIndex(2) == 0) {
    mIsDolphin[aDolphin.mX][aDolphin.mY] = 0U;
    aDolphin.mDeadFlag = true;
    ++mRuntimeStats.mDolphinDeaths;
    return;
  }

  mIsShark[aShark.mX][aShark.mY] = 0U;
  ++mRuntimeStats.mDolphinSharkKills;
  ++mProbeStats.mSharkKillCount;
  mProbeStats.mTotalSharkMovesBeforeKill += static_cast<std::uint64_t>(mSharkMovesSinceKill[pSharkIndex]);
  aShark.mDeadFlag = true;
  mSharkMovesSinceKill[pSharkIndex] = 0;
}

void MazeDirector::MarkRobotAndSharkDead(int pRobotIndex, int pSharkIndex) {
  if (pRobotIndex < 0 || pRobotIndex >= mRobotListCount || pSharkIndex < 0 ||
      pSharkIndex >= mSharkListCount || mRobotList[pRobotIndex] == nullptr ||
      mSharkList[pSharkIndex] == nullptr) {
    return;
  }

  MazeRobot& aRobot = *mRobotList[pRobotIndex];
  MazeShark& aShark = *mSharkList[pSharkIndex];
  if (!aShark.mDeadFlag) {
    if (InBounds(aShark.mX, aShark.mY)) {
      mIsShark[aShark.mX][aShark.mY] = 0U;
    }
    ++mProbeStats.mSharkKillCount;
    mProbeStats.mTotalSharkMovesBeforeKill += static_cast<std::uint64_t>(mSharkMovesSinceKill[pSharkIndex]);
    aShark.mDeadFlag = true;
    mSharkMovesSinceKill[pSharkIndex] = 0;
  }
  if (!aRobot.mDeadFlag) {
    ++mRuntimeStats.mDeaths;
    aRobot.Die();
  }
}

void MazeDirector::PickDistinctRows(int* pRows, int pRowCount) {
  if (pRows == nullptr || pRowCount <= 0) {
    return;
  }

  bool aPicked[kGridHeight] = {};
  int aCount = 0;
  const int aLimit = (pRowCount < kGridHeight) ? pRowCount : kGridHeight;
  while (aCount < aLimit) {
    const int aRow = NextIndex(kGridHeight);
    if (aPicked[aRow]) {
      continue;
    }
    aPicked[aRow] = true;
    pRows[aCount] = aRow;
    ++aCount;
  }
}

void MazeDirector::PickDistinctCols(int* pCols, int pColCount) {
  if (pCols == nullptr || pColCount <= 0) {
    return;
  }

  bool aPicked[kGridWidth] = {};
  int aCount = 0;
  const int aLimit = (pColCount < kGridWidth) ? pColCount : kGridWidth;
  while (aCount < aLimit) {
    const int aCol = NextIndex(kGridWidth);
    if (aPicked[aCol]) {
      continue;
    }
    aPicked[aCol] = true;
    pCols[aCount] = aCol;
    ++aCount;
  }
}

void MazeDirector::HandleSpecialEvents() {
  if (NextIndex(256) == 128 && NextIndex(256) < 16) {
    MazeSpecialEvents::StarBurst(*this, mRobotList, mRobotListCount);
  }
  if (NextIndex(256) == 76 && NextIndex(256) < 16) {
    MazeSpecialEvents::ChaosStorm(*this, mSharkList, mSharkListCount);
  }
  if (NextIndex(256) == 80 && NextIndex(256) < 8) {
    int aRows[3] = {};
    PickDistinctRows(aRows, 3);
    MazeSpecialEvents::CometTrailsHorizontal(*this, aRows, 3);
  }
  if (NextIndex(256) == 80 && NextIndex(256) < 8) {
    int aCols[3] = {};
    PickDistinctCols(aCols, 3);
    MazeSpecialEvents::CometTrailsVertical(*this, aCols, 3);
  }
}

bool MazeDirector::MoveSharks() {
  for (int aSharkIndex = 0; aSharkIndex < mSharkListCount; ++aSharkIndex) {
    MazeShark& aShark = *mSharkList[aSharkIndex];
    if (aShark.mDeadFlag) {
      continue;
    }
    if (aShark.mDidRecentlyRevive) {
      aShark.mDidRecentlyRevive = false;
      continue;
    }
    MoveShark(aShark, aSharkIndex);
  }
  return true;
}

bool MazeDirector::MoveDolphins() {
  for (int aDolphinIndex = 0; aDolphinIndex < mDolphinListCount; ++aDolphinIndex) {
    MazeDolphin& aDolphin = *mDolphinList[aDolphinIndex];
    if (aDolphin.mDeadFlag) {
      continue;
    }
    if (aDolphin.mDidRecentlyRevive) {
      aDolphin.mDidRecentlyRevive = false;
      continue;
    }

    MoveDolphin(aDolphin, aDolphinIndex);
    (void)RepaintOrFlushTile(aDolphin.mX, aDolphin.mY);
    if (!MarkDolphinCheeseSteal(aDolphinIndex)) {
      return false;
    }
  }

  return true;
}

void MazeDirector::MarkRobotSharkCollisions() {
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    MazeRobot& aRobot = *mRobotList[aRobotIndex];
    if (aRobot.mDeadFlag || !InBounds(aRobot.mX, aRobot.mY)) {
      continue;
    }
    const int aSharkIndex = FindSharkAt(aRobot.mX, aRobot.mY);
    if (aSharkIndex >= 0) {
      MarkRobotAndSharkDead(aRobotIndex, aSharkIndex);
    }
  }
}

void MazeDirector::MarkDolphinSharkCollisions() {
  for (int aDolphinIndex = 0; aDolphinIndex < mDolphinListCount; ++aDolphinIndex) {
    MazeDolphin& aDolphin = *mDolphinList[aDolphinIndex];
    if (aDolphin.mDeadFlag || !InBounds(aDolphin.mX, aDolphin.mY)) {
      continue;
    }
    const int aSharkIndex = FindSharkAt(aDolphin.mX, aDolphin.mY);
    if (aSharkIndex >= 0) {
      MarkDolphinAndSharkDead(aDolphinIndex, aSharkIndex);
    }
  }
}

bool MazeDirector::ResolveCapturedCheeses() {
  for (int aCheeseIndex = 0; aCheeseIndex < mCheeseListCount; ++aCheeseIndex) {
    if (!mCapturedCheeseThisPulse[aCheeseIndex] || mCheeseList[aCheeseIndex] == nullptr) {
      continue;
    }

    MazeCheese& aCheese = *mCheeseList[aCheeseIndex];
    for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
      MazeRobot* aRobot = mRobotList[aRobotIndex];
      if (aRobot == nullptr || aRobot->mCheese != &aCheese) {
        continue;
      }
      aRobot->mCheese = nullptr;
      aRobot->mNeedsRepath = !aRobot->mDeadFlag && !aRobot->mIsVictorious;
      (void)aRobot->AssignPathAndStartWalk(nullptr, nullptr, 0);
    }

    if (!RespawnCheese(aCheeseIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  return true;
}

bool MazeDirector::ResolveDeadEntities() {
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    MazeRobot& aRobot = *mRobotList[aRobotIndex];
    if (!aRobot.mDeadFlag && !aRobot.mIsVictorious) {
      continue;
    }
    FinalizeRobotLife(aRobotIndex);
    if (!RespawnRobot(aRobotIndex, true)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
    aRobot.mIsVictorious = false;
    aRobot.mNeedsRepath = true;
  }

  for (int aSharkIndex = 0; aSharkIndex < mSharkListCount; ++aSharkIndex) {
    if (!mSharkList[aSharkIndex]->mDeadFlag) {
      continue;
    }
    if (!RespawnShark(aSharkIndex, true)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aDolphinIndex = 0; aDolphinIndex < mDolphinListCount; ++aDolphinIndex) {
    if (!mDolphinList[aDolphinIndex]->mDeadFlag) {
      continue;
    }
    if (!RespawnDolphin(aDolphinIndex, true)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  return true;
}

bool MazeDirector::ResolveRobotRepaths() {
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    MazeRobot& aRobot = *mRobotList[aRobotIndex];
    if (aRobot.mDeadFlag || aRobot.mIsVictorious) {
      continue;
    }

    const bool aNeedsPath = aRobot.mNeedsRepath || aRobot.mCheese == nullptr || aRobot.mPathLength <= 0 ||
                            aRobot.mPathIndex < 0 || aRobot.mPathIndex >= aRobot.mPathLength;
    if (!aNeedsPath) {
      continue;
    }

    if (!AssignPathableCheese(&aRobot)) {
      ++mRuntimeStats.mInconsistentStateB;
      return false;
    }
    aRobot.mNeedsRepath = false;
    aRobot.mDidRecentlyRepath = true;
    RepaintRobotSquare(aRobot.mX, aRobot.mY);
  }

  return true;
}

bool MazeDirector::MoveRobots() {
  for (int aRobotIndex = 0; aRobotIndex < mRobotListCount; ++aRobotIndex) {
    MazeRobot& aRobot = *mRobotList[aRobotIndex];
    if (aRobot.mDeadFlag || aRobot.mIsVictorious) {
      continue;
    }
    if (aRobot.mCheese == nullptr || aRobot.mPathLength <= 0 || aRobot.mPathIndex < 0) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }
    if (aRobot.mPathIndex >= aRobot.mPathLength) {
      bool aCaptureFailure = false;
      if (!MarkRobotVictory(aRobotIndex, &aCaptureFailure)) {
        if (aCaptureFailure) {
          return false;
        }
        ++mRuntimeStats.mInconsistentStateH;
        return false;
      }
      continue;
    }
    if (aRobot.mDidRecentlyRepath) {
      aRobot.mDidRecentlyRepath = false;
      continue;
    }

    if (!aRobot.Update(static_cast<const MazeGrid&>(*this))) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }

    bool aCaptureFailure = false;
    int aNextX = aRobot.mX;
    int aNextY = aRobot.mY;
    if (!aRobot.ReadPendingStep(0, &aNextX, &aNextY)) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }

    aRobot.ApplyPendingStep(0);
    ++mRobotWalks[aRobotIndex];
    ApplyRobotPowerUp(aRobotIndex);

    if (MarkRobotVictory(aRobotIndex, &aCaptureFailure)) {
      aRobot.FinishUpdate();
      continue;
    }
    if (aCaptureFailure || aRobot.mDeadFlag) {
      return false;
    }

    aRobot.FinishUpdate();

    if (!aRobot.mDeadFlag && !aRobot.mIsVictorious && aRobot.mCheese != nullptr && aRobot.mPathLength > 0 &&
        aRobot.mPathIndex > aRobot.mPathLength) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }

    if (!aRobot.mDeadFlag && !aRobot.mIsVictorious) {
      RepaintRobotSquare(aRobot.mX, aRobot.mY);
    }
  }

  return true;
}

void MazeDirector::Build() {
  switch (ResolveGenerationMode()) {
    case GenerationMode::kCustom:
      GenerateCustom();
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
    ++mRuntimeStats.mInconsistentStateC;
    ClearWalls();
    FinalizeWalls();
  }
}

}  // namespace peanutbutter::maze
