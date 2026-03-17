#include "MazeDirector.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include "MazeSpecialEvents.hpp"

namespace peanutbutter::maze {

namespace {

using helpers::MazeCheese;
using helpers::MazeRobot;
using helpers::MazeShark;
using helpers::PowerUpType;

constexpr int kCheeseCooldownPulses = 4;
constexpr int kRobotConfusedLimit = 10;

}  // namespace

MazeDirector::MazeDirector()
    : Maze(),
      mFastRand(),
      mProbeConfig{},
      mProbeStats{},
      mGameIndex(0),
      mGameName(GetProbeConfigNameByIndex(0)),
      mRobots{},
      mCheeses{},
      mSharks{},
      mIsShark{},
      mPowerUpType{},
      mRobotWalks{},
      mSharkMovesSinceKill{},
      mSharkMoveCandidateX{},
      mSharkMoveCandidateY{},
      mSharkMoveCandidateCount(0),
      mRobotLifeActive{} {
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
  mRobots = {};
  mCheeses = {};
  mSharks = {};
  std::memset(mIsShark, 0, sizeof(mIsShark));
  std::memset(mPowerUpType, 0, sizeof(mPowerUpType));
  std::memset(mRobotWalks, 0, sizeof(mRobotWalks));
  std::memset(mSharkMovesSinceKill, 0, sizeof(mSharkMovesSinceKill));
  std::memset(mSharkMoveCandidateX, 0, sizeof(mSharkMoveCandidateX));
  std::memset(mSharkMoveCandidateY, 0, sizeof(mSharkMoveCandidateY));
  mSharkMoveCandidateCount = 0;
  std::memset(mRobotLifeActive, 0, sizeof(mRobotLifeActive));
}

bool MazeDirector::IsTileOccupied(int pX,
                                  int pY,
                                  int pIgnoreRobotIndex,
                                  int pIgnoreCheeseIndex,
                                  int pIgnoreSharkIndex) const {
  if (!InBounds(pX, pY)) {
    return true;
  }

  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  const int aCheeseCount = ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses);
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);

  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    if (aRobotIndex == pIgnoreRobotIndex || mRobots[aRobotIndex].mDead) {
      continue;
    }
    if (mRobots[aRobotIndex].mX == pX && mRobots[aRobotIndex].mY == pY) {
      return true;
    }
  }

  for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
    if (aCheeseIndex == pIgnoreCheeseIndex || !mCheeses[aCheeseIndex].mIsValid) {
      continue;
    }
    if (mCheeses[aCheeseIndex].mX == pX && mCheeses[aCheeseIndex].mY == pY) {
      return true;
    }
  }

  for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
    if (aSharkIndex == pIgnoreSharkIndex || mSharks[aSharkIndex].mDead) {
      continue;
    }
    if (mSharks[aSharkIndex].mX == pX && mSharks[aSharkIndex].mY == pY) {
      return true;
    }
  }

  return mPowerUpType[pX][pY] != 0U;
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
  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    FinalizeRobotLife(aRobotIndex);
  }
}

bool MazeDirector::RespawnRobot(int pRobotIndex) {
  if (pRobotIndex < 0 || pRobotIndex >= helpers::kMaxRobots) {
    return false;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, pRobotIndex, -1, -1); }, &aRespawnX, &aRespawnY)) {
    return false;
  }

  MazeRobot& aRobot = mRobots[pRobotIndex];
  aRobot = MazeRobot{};
  aRobot.mX = aRespawnX;
  aRobot.mY = aRespawnY;
  aRobot.mDead = false;
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
    return false;
  }

  MazeCheese& aCheese = mCheeses[pCheeseIndex];
  aCheese.mX = aRespawnX;
  aCheese.mY = aRespawnY;
  aCheese.mRespawnTick = 0;
  aCheese.mIsValid = true;
  return true;
}

bool MazeDirector::RespawnShark(int pSharkIndex) {
  if (pSharkIndex < 0 || pSharkIndex >= helpers::kMaxSharks) {
    return false;
  }

  MazeShark& aShark = mSharks[pSharkIndex];
  if (!aShark.mDead && InBounds(aShark.mX, aShark.mY)) {
    mIsShark[aShark.mX][aShark.mY] = 0U;
  }

  int aRespawnX = -1;
  int aRespawnY = -1;
  if (!helpers::PickUniqueWalkable(
          [&](int& pX, int& pY) { return GetRandomWalkable(pX, pY); },
          [&](int pX, int pY) { return IsTileOccupied(pX, pY, -1, -1, pSharkIndex); }, &aRespawnX, &aRespawnY)) {
    return false;
  }

  aShark.mX = aRespawnX;
  aShark.mY = aRespawnY;
  aShark.mDead = false;
  mIsShark[aShark.mX][aShark.mY] = 1U;
  mSharkMovesSinceKill[pSharkIndex] = 0;
  return true;
}

bool MazeDirector::InitializePulseState() {
  ResetPulseState();

  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  const int aCheeseCount = ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses);
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);

  for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
    if (!RespawnCheese(aCheeseIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    if (!RespawnRobot(aRobotIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
    if (!RespawnShark(aSharkIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  for (int aIndex = 0; aIndex < mWalkableListCount; ++aIndex) {
    const int aX = mWalkableListX[aIndex];
    const int aY = mWalkableListY[aIndex];
    if (IsTileOccupied(aX, aY) || NextIndex(helpers::kPowerUpSpawnDivisor) != 0) {
      continue;
    }
    mPowerUpType[aX][aY] = static_cast<unsigned char>(NextIndex(4) + 1);
  }

  return true;
}

int MazeDirector::FindSharkAt(int pX, int pY) const {
  if (!InBounds(pX, pY) || mIsShark[pX][pY] == 0U) {
    return -1;
  }
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);
  for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
    if (mSharks[aSharkIndex].mDead) {
      continue;
    }
    if (mSharks[aSharkIndex].mX == pX && mSharks[aSharkIndex].mY == pY) {
      return aSharkIndex;
    }
  }
  return -1;
}

void MazeDirector::MoveShark(MazeShark& pShark, int pSharkIndex) {
  if (pSharkIndex < 0 || pSharkIndex >= helpers::kMaxSharks || pShark.mDead || !InBounds(pShark.mX, pShark.mY)) {
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

MazeCheese* MazeDirector::PickRandomValidCheese() {
  return helpers::ChooseValidCheese([&](int pLimit) { return NextIndex(pLimit); }, &mCheeses,
                                    ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses));
}

bool MazeDirector::StoreRobotPath(MazeRobot* pRobot, MazeCheese* pCheese) {
  if (pRobot == nullptr || pCheese == nullptr || pRobot->mDead || !pCheese->mIsValid) {
    return false;
  }

  pRobot->mTargetCheese = nullptr;
  pRobot->mPathLength = 0;
  pRobot->mPathIndex = 0;

  if (!FindPath(pRobot->mX, pRobot->mY, pCheese->mX, pCheese->mY) || PathLength() <= 0) {
    ++mRuntimeStats.mInconsistentStateE;
    return false;
  }

  pRobot->mTargetCheese = pCheese;
  pRobot->mPathLength = PathLength();
  for (int aPathIndex = 0; aPathIndex < pRobot->mPathLength; ++aPathIndex) {
    if (!PathNode(aPathIndex, &pRobot->mPathX[aPathIndex], &pRobot->mPathY[aPathIndex])) {
      pRobot->mTargetCheese = nullptr;
      pRobot->mPathLength = 0;
      pRobot->mPathIndex = 0;
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }
  }

  pRobot->mPathIndex = (pRobot->mPathLength > 1) ? 1 : 0;
  return true;
}

bool MazeDirector::AssignPathableCheese(MazeRobot* pRobot) {
  if (pRobot == nullptr || pRobot->mDead) {
    return false;
  }

  MazeCheese* aCheese = PickRandomValidCheese();
  if (aCheese == nullptr) {
    pRobot->mTargetCheese = nullptr;
    pRobot->mPathLength = 0;
    pRobot->mPathIndex = 0;
    return false;
  }
  return StoreRobotPath(pRobot, aCheese);
}

bool MazeDirector::RepaintOrFlushTile(int pX, int pY) {
  if (IsWall(pX, pY)) {
    return false;
  }

  const bool aHadByte = (mIsByte[pX][pY] != 0);
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
  if (pRobotIndex < 0 || pRobotIndex >= helpers::kMaxRobots) {
    return;
  }

  MazeRobot& aRobot = mRobots[pRobotIndex];
  if (aRobot.mDead || !InBounds(aRobot.mX, aRobot.mY)) {
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

bool MazeDirector::CaptureCheese(int pRobotIndex) {
  if (pRobotIndex < 0 || pRobotIndex >= helpers::kMaxRobots) {
    return false;
  }

  MazeRobot& aRobot = mRobots[pRobotIndex];
  if (aRobot.mDead || aRobot.mVictorious) {
    return false;
  }

  const int aCheeseCount = ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses);
  for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
    MazeCheese& aCheese = mCheeses[aCheeseIndex];
    if (!aCheese.mIsValid) {
      continue;
    }
    if (AbsInt(aCheese.mX - aRobot.mX) > 1 || AbsInt(aCheese.mY - aRobot.mY) > 1) {
      continue;
    }

    aCheese.mIsValid = false;
    aCheese.mRespawnTick = kCheeseCooldownPulses + 1;
    aRobot.mVictorious = true;
    aRobot.mTargetCheese = nullptr;
    aRobot.mWasConfused = false;
    aRobot.mConfusedTick = 0;
    aRobot.mPowerUp = PowerUpType::kNone;
    aRobot.mPowerUpTick = 0;
    ++mRuntimeStats.mVictories;
    FinalizeRobotLife(pRobotIndex);
    return true;
  }

  return false;
}

void MazeDirector::RecordRobotAction(int* pActionCount, int* pOutActions) {
  if (pActionCount == nullptr) {
    return;
  }

  ++(*pActionCount);
  if (pOutActions != nullptr) {
    *pOutActions = *pActionCount;
  }
}

void MazeDirector::TriggerConfusedRobot(MazeRobot* pRobot) {
  if (pRobot == nullptr) {
    return;
  }

  ++mRuntimeStats.mInconsistentStateB;
  pRobot->mTargetCheese = nullptr;
  pRobot->mPathLength = 0;
  pRobot->mPathIndex = 0;
  if (!pRobot->mWasConfused) {
    pRobot->mWasConfused = true;
    pRobot->mConfusedTick = kRobotConfusedLimit;
    return;
  }
  if (pRobot->mConfusedTick > 0) {
    --pRobot->mConfusedTick;
    if (pRobot->mConfusedTick == 0) {
      ++mRuntimeStats.mInconsistentStateD;
    }
  }
}

void MazeDirector::KillRobotAndShark(int pRobotIndex, int pSharkIndex) {
  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);
  if (pRobotIndex < 0 || pRobotIndex >= aRobotCount || pSharkIndex < 0 || pSharkIndex >= aSharkCount) {
    return;
  }

  MazeRobot& aRobot = mRobots[pRobotIndex];
  MazeShark& aShark = mSharks[pSharkIndex];
  if (!aShark.mDead) {
    if (InBounds(aShark.mX, aShark.mY)) {
      mIsShark[aShark.mX][aShark.mY] = 0U;
    }
    ++mProbeStats.mSharkKillCount;
    mProbeStats.mTotalSharkMovesBeforeKill += static_cast<std::uint64_t>(mSharkMovesSinceKill[pSharkIndex]);
    aShark.mDead = true;
    mSharkMovesSinceKill[pSharkIndex] = 0;
  }
  if (!aRobot.mDead) {
    ++mRuntimeStats.mDeaths;
    aRobot.mDead = true;
    aRobot.mVictorious = false;
    aRobot.mTargetCheese = nullptr;
    aRobot.mPathLength = 0;
    aRobot.mPathIndex = 0;
    aRobot.mWasConfused = false;
    aRobot.mConfusedTick = 0;
    aRobot.mPowerUp = PowerUpType::kNone;
    aRobot.mPowerUpTick = 0;
    FinalizeRobotLife(pRobotIndex);
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

void MazeDirector::TriggerSpecialEvents() {
  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);

  if (NextIndex(256) == 128 && NextIndex(256) < 16) {
    MazeSpecialEvents::StarBurst(*this, mRobots, aRobotCount);
  }
  if (NextIndex(256) == 76 && NextIndex(256) < 16) {
    MazeSpecialEvents::ChaosStorm(*this, mSharks, aSharkCount);
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
  const int aSharkCount = ClampConfiguredCount(mProbeConfig.mSharkCount, helpers::kMaxSharks);
  for (int aSharkIndex = 0; aSharkIndex < aSharkCount; ++aSharkIndex) {
    MazeShark& aShark = mSharks[aSharkIndex];
    if (aShark.mDead) {
      if (!RespawnShark(aSharkIndex)) {
        ++mRuntimeStats.mInconsistentStateA;
        return false;
      }
      continue;
    }
    MoveShark(aShark, aSharkIndex);
  }
  return true;
}

void MazeDirector::CollideRobotsWithSharks() {
  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    MazeRobot& aRobot = mRobots[aRobotIndex];
    if (aRobot.mDead || !InBounds(aRobot.mX, aRobot.mY)) {
      continue;
    }
    const int aSharkIndex = FindSharkAt(aRobot.mX, aRobot.mY);
    if (aSharkIndex >= 0) {
      KillRobotAndShark(aRobotIndex, aSharkIndex);
    }
  }
}

bool MazeDirector::ManageRobots(bool* pRobotCanMove, int* pActionCount, int* pOutActions) {
  if (pRobotCanMove == nullptr) {
    return false;
  }

  std::fill_n(pRobotCanMove, helpers::kMaxRobots, false);
  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    MazeRobot& aRobot = mRobots[aRobotIndex];
    if (aRobot.mDead || aRobot.mVictorious) {
      if (!RespawnRobot(aRobotIndex)) {
        ++mRuntimeStats.mInconsistentStateA;
        return false;
      }
      RepaintRobotSquare(aRobot.mX, aRobot.mY);
      RecordRobotAction(pActionCount, pOutActions);
      continue;
    }

    if (aRobot.mTargetCheese == nullptr ||
        !aRobot.mTargetCheese->mIsValid ||
        aRobot.mPathLength <= 0 ||
        aRobot.mPathIndex < 0 ||
        aRobot.mPathIndex > aRobot.mPathLength) {
      if (!AssignPathableCheese(&aRobot)) {
        TriggerConfusedRobot(&aRobot);
        RepaintRobotSquare(aRobot.mX, aRobot.mY);
        RecordRobotAction(pActionCount, pOutActions);
        continue;
      }
    }

    aRobot.mWasConfused = false;
    aRobot.mConfusedTick = 0;
    pRobotCanMove[aRobotIndex] = true;
  }

  return true;
}

bool MazeDirector::MoveRobots(const bool* pRobotCanMove, int* pActionCount, int* pOutActions) {
  if (pRobotCanMove == nullptr) {
    return false;
  }

  const int aRobotCount = ClampConfiguredCount(mProbeConfig.mRobotCount, helpers::kMaxRobots);
  for (int aRobotIndex = 0; aRobotIndex < aRobotCount; ++aRobotIndex) {
    if (!pRobotCanMove[aRobotIndex]) {
      continue;
    }

    MazeRobot& aRobot = mRobots[aRobotIndex];
    int aPaintX = aRobot.mX;
    int aPaintY = aRobot.mY;

    if (aRobot.mDead || aRobot.mVictorious) {
      continue;
    }

    if (aRobot.mTargetCheese == nullptr || !aRobot.mTargetCheese->mIsValid) {
      if (!AssignPathableCheese(&aRobot)) {
        TriggerConfusedRobot(&aRobot);
        RepaintRobotSquare(aPaintX, aPaintY);
        RecordRobotAction(pActionCount, pOutActions);
        continue;
      }
      aRobot.mWasConfused = false;
      aRobot.mConfusedTick = 0;
    }

    if (aRobot.mPathLength <= 0 || aRobot.mPathIndex < 0 || aRobot.mPathIndex > aRobot.mPathLength) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }

    int aHopBudget = 1;
    if (aRobot.mPowerUp == PowerUpType::kTeleport && aRobot.mPowerUpTick > 0) {
      aHopBudget = 4;
    } else if (aRobot.mPowerUp == PowerUpType::kSuperSpeed && aRobot.mPowerUpTick > 0) {
      aHopBudget = 2;
    }

    for (int aHop = 0; aHop < aHopBudget; ++aHop) {
      if (aRobot.mPathIndex <= 0 && aRobot.mPathLength > 1) {
        aRobot.mPathIndex = 1;
      }
      if (aRobot.mPathIndex < 0 || aRobot.mPathIndex >= aRobot.mPathLength) {
        break;
      }

      const int aNextX = aRobot.mPathX[aRobot.mPathIndex];
      const int aNextY = aRobot.mPathY[aRobot.mPathIndex];
      ++aRobot.mPathIndex;

      aRobot.mX = aNextX;
      aRobot.mY = aNextY;
      aPaintX = aNextX;
      aPaintY = aNextY;
      ++mRobotWalks[aRobotIndex];

      const int aSharkIndex = FindSharkAt(aRobot.mX, aRobot.mY);
      if (aSharkIndex >= 0) {
        KillRobotAndShark(aRobotIndex, aSharkIndex);
        break;
      }

      ApplyRobotPowerUp(aRobotIndex);
      if (CaptureCheese(aRobotIndex) || aRobot.mDead || aRobot.mVictorious) {
        break;
      }
    }

    if (!aRobot.mDead && !aRobot.mVictorious) {
      (void)CaptureCheese(aRobotIndex);
    }

    if (!aRobot.mDead &&
        !aRobot.mVictorious &&
        aRobot.mTargetCheese != nullptr &&
        aRobot.mTargetCheese->mIsValid &&
        aRobot.mPathLength > 0 &&
        aRobot.mPathIndex >= aRobot.mPathLength) {
      ++mRuntimeStats.mInconsistentStateE;
      return false;
    }

    if (!aRobot.mDead && aRobot.mPowerUpTick > 0) {
      --aRobot.mPowerUpTick;
      if (aRobot.mPowerUpTick <= 0) {
        aRobot.mPowerUpTick = 0;
        aRobot.mPowerUp = PowerUpType::kNone;
      }
    }

    RepaintRobotSquare(aPaintX, aPaintY);
    RecordRobotAction(pActionCount, pOutActions);
  }

  return true;
}

bool MazeDirector::ManageCheese() {
  const int aCheeseCount = ClampConfiguredCount(mProbeConfig.mCheeseCount, helpers::kMaxCheeses);
  for (int aCheeseIndex = 0; aCheeseIndex < aCheeseCount; ++aCheeseIndex) {
    MazeCheese& aCheese = mCheeses[aCheeseIndex];
    if (aCheese.mIsValid) {
      continue;
    }
    if (aCheese.mRespawnTick > 0) {
      --aCheese.mRespawnTick;
      if (aCheese.mRespawnTick > 0) {
        continue;
      }
    }
    if (!RespawnCheese(aCheeseIndex)) {
      ++mRuntimeStats.mInconsistentStateA;
      return false;
    }
  }

  return true;
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

bool MazeDirector::PathingLoopPulse(int* pOutActions) {
  if (pOutActions != nullptr) {
    *pOutActions = 0;
  }

  int aActions = 0;
  bool aRobotCanMove[helpers::kMaxRobots] = {};

  TriggerSpecialEvents();
  if (!MoveSharks()) {
    return false;
  }
  CollideRobotsWithSharks();
  if (!ManageRobots(aRobotCanMove, &aActions, pOutActions)) {
    return false;
  }
  if (!MoveRobots(aRobotCanMove, &aActions, pOutActions)) {
    return false;
  }
  CollideRobotsWithSharks();
  if (!ManageCheese()) {
    return false;
  }

  return true;
}

bool MazeDirector::SimulationMainLoop() {
  if (!Generate()) {
    return false;
  }

  int aActionsThisMaze = 0;
  while (aActionsThisMaze < kMazeRespawnActionThreshold && mResultBufferWriteProgress < mResultBufferLength) {
    int aPulseActions = 0;
    if (!PathingLoopPulse(&aPulseActions)) {
      return false;
    }
    aActionsThisMaze += aPulseActions;
    if (aPulseActions <= 0) {
      break;
    }
  }

  FinalizePulseStats();
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
