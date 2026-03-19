#include "GameBoard.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "GamePlayDirector.hpp"
#include "GamePowerUp.hpp"

namespace peanutbutter::games {

namespace {
constexpr int kVeryLargeEnsureIterations = 4096;

GamePowerUp_ZoneBomb gZoneBomb;
GamePowerUp_Rocket gRocket;
GamePowerUp_ColorBomb gColorBomb;
GamePowerUp_CrossBomb gCrossBomb;
GamePowerUp_PlasmaBeamH gPlasmaBeamH;
GamePowerUp_PlasmaBeamV gPlasmaBeamV;
GamePowerUp_PlasmaBeamQuad gPlasmaBeamQuad;
GamePowerUp_Nuke gNuke;
GamePowerUp_CornerBomb gCornerBomb;
GamePowerUp_VerticalBombs gVerticalBombs;
GamePowerUp_HorizontalBombs gHorizontalBombs;

GamePowerUpType SelectPowerUpType(unsigned char pSeedByte) {
  const unsigned char aPick = static_cast<unsigned char>(pSeedByte % 11U);
  if (aPick == 0U) {
    return GamePowerUpType::kZoneBomb;
  }
  if (aPick == 1U) {
    return GamePowerUpType::kRocket;
  }
  if (aPick == 2U) {
    return GamePowerUpType::kColorBomb;
  }
  if (aPick == 3U) {
    return GamePowerUpType::kCrossBomb;
  }
  if (aPick == 4U) {
    return GamePowerUpType::kPlasmaBeamH;
  }
  if (aPick == 5U) {
    return GamePowerUpType::kPlasmaBeamV;
  }
  if (aPick == 6U) {
    return GamePowerUpType::kPlasmaBeamQuad;
  }
  if (aPick == 7U) {
    return GamePowerUpType::kNuke;
  }
  if (aPick == 8U) {
    return GamePowerUpType::kCornerBomb;
  }
  if (aPick == 9U) {
    return GamePowerUpType::kVerticalBombs;
  }
  return GamePowerUpType::kHorizontalBombs;
}

}  // namespace

GameBoard::GameBoard()
    : mGrid{},
      mOwnedTiles(new GameTile[kTilePoolCapacity]),
      mTileStack(nullptr),
      mFastRand(),
      mGamePlayDirector(nullptr),
      mMoveBehavior(kTap),
      mMovePolicy(kRandom),
      mMatchBehavior(kStreak),
      mIsCascading(false),
      mCataclysmWriteIndex(0U),
      mApocalypseWriteIndex(0U),
      mSuccessfulMoveCount(0),
      mBrokenCount(0),
      mGameIndex(IslandSwapFourGreedy),
      mTileTypeCount(4),
      mGameName("Starlight Scramble [IS4G]"),
      mMoveListX{},
      mMoveListY{},
      mMoveListHorizontal{},
      mMoveListDir{},
      mMoveListScore{},
      mMoveListCount(0),
      mExploreListX{},
      mExploreListY{},
      mExploreListCount(0),
      mCandidateListX{},
      mCandidateListY{},
      mCandidateListCount(0),
      mViolatingListX{},
      mViolatingListY{},
      mViolatingListCount(0),
      mToppleSearchCount(0),
      mToppleChanged{},
      mToppleSearchMask{},
      mCheckStackX{},
      mCheckStackY{},
      mCheckStackCount(0),
      mCheckVisited{},
      mMatchListX{},
      mMatchListY{},
      mMatchListCount(0),
      mStackX{},
      mStackY{},
      mComponentX{},
      mComponentY{},
      mVisited{},
      mRuntimeStats{} {
  std::memset(mGrid, 0, sizeof(mGrid));
  for (int aIndex = kTilePoolCapacity - 1; aIndex >= 0; --aIndex) {
    mOwnedTiles[aIndex].mNext = mTileStack;
    mTileStack = &mOwnedTiles[aIndex];
  }
  ConfigureGameByIndex(IslandSwapFourGreedy);
}

GameBoard::~GameBoard() {
  ClearBoard();
  delete[] mOwnedTiles;
  mOwnedTiles = nullptr;
  mTileStack = nullptr;
}

void GameBoard::InitializeSeed(unsigned char* pPassword, int pPasswordLength) {
  if (pPasswordLength != kSeedBufferCapacity) {
    std::abort();
  }
  ClearBoard();
  mSuccessfulMoveCount = 0;
  mBrokenCount = 0;
  mMoveListCount = 0;
  mExploreListCount = 0;
  mCandidateListCount = 0;
  mViolatingListCount = 0;
  mMatchListCount = 0;
  mRuntimeStats = RuntimeStats{};

  InitializeSeedBuffer(pPassword, pPasswordLength);
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mFastRand.Seed(mSeedBuffer, pPasswordLength);
}

void GameBoard::SetMoveBehavior(MoveBehavior pMoveBehavior) {
  mMoveBehavior = pMoveBehavior;
}

void GameBoard::SetMovePolicy(MovePolicy pMovePolicy) {
  mMovePolicy = pMovePolicy;
}

void GameBoard::SetMatchBehavior(MatchBehavior pMatchBehavior) {
  mMatchBehavior = pMatchBehavior;
}

void GameBoard::SetIsCascading(bool pIsCascading) {
  mIsCascading = pIsCascading;
}

void GameBoard::SetGamePlayDirector(GamePlayDirector* pGamePlayDirector) {
  mGamePlayDirector = pGamePlayDirector;
}

bool GameBoard::ResolveBoardState_PostSpawn() {
  if (mGamePlayDirector == nullptr) {
    return false;
  }
  return mGamePlayDirector->ResolveBoardState_PostSpawn(this);
}

bool GameBoard::ResolveBoardState_PostTopple() {
  if (mGamePlayDirector == nullptr) {
    return false;
  }
  return mGamePlayDirector->ResolveBoardState_PostTopple(this);
}

void GameBoard::RecordGameStateOverflowCatastrophic(CatastropheCase pCase) {
  ++mRuntimeStats.mGameStateOverflowCatastrophic;
  switch (pCase) {
    case kCatastropheCaseA:
      ++mRuntimeStats.mCatastropheCaseA;
      break;
    case kCatastropheCaseB:
      ++mRuntimeStats.mCatastropheCaseB;
      break;
    case kCatastropheCaseC:
    default:
      ++mRuntimeStats.mCatastropheCaseC;
      break;
  }
}

void GameBoard::RecordGameStateOverflowCataclysmic() {
  ++mRuntimeStats.mGameStateOverflowCataclysmic;
}

void GameBoard::RecordGameStateOverflowApocalypse() {
  ++mRuntimeStats.mGameStateOverflowApocalypse;
}

void GameBoard::ShuffleSeedBuffer() {
  if (mResultBufferLength <= 1U) {
    return;
  }

  for (unsigned int aIndex = mResultBufferLength - 1U; aIndex > 0U; --aIndex) {
    const unsigned int aSwapIndex = static_cast<unsigned int>(GetRand(static_cast<int>(aIndex + 1U)));
    const unsigned char aTemp = mSeedBuffer[aIndex];
    mSeedBuffer[aIndex] = mSeedBuffer[aSwapIndex];
    mSeedBuffer[aSwapIndex] = aTemp;
  }
}

void GameBoard::DragonAttack() {
  bool aAny = false;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr || IsTileMatched(aX, aY)) {
        continue;
      }
      if (GetRand(3) == 0) {
        MarkTileMatched(aX, aY);
        aAny = true;
      }
    }
  }
  (void)aAny;
}

void GameBoard::PhoenixAttack() {
  int aColumnOrder[kGridWidth] = {};
  for (int aIndex = 0; aIndex < kGridWidth; ++aIndex) {
    aColumnOrder[aIndex] = aIndex;
  }
  for (int aIndex = kGridWidth - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = static_cast<int>(GetRand(aIndex + 1));
    const int aTemp = aColumnOrder[aIndex];
    aColumnOrder[aIndex] = aColumnOrder[aSwapIndex];
    aColumnOrder[aSwapIndex] = aTemp;
  }

  const int aColumnCount = std::min(4, kGridWidth);
  for (int aPickIndex = 0; aPickIndex < aColumnCount; ++aPickIndex) {
    const int aColumn = aColumnOrder[aPickIndex];
    for (int aY = 0; aY < kGridHeight; ++aY) {
      MarkTileMatched(aColumn, aY);
    }
  }
}

void GameBoard::WyvernAttack() {
  int aRowOrder[kGridHeight] = {};
  for (int aIndex = 0; aIndex < kGridHeight; ++aIndex) {
    aRowOrder[aIndex] = aIndex;
  }
  for (int aIndex = kGridHeight - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = static_cast<int>(GetRand(aIndex + 1));
    const int aTemp = aRowOrder[aIndex];
    aRowOrder[aIndex] = aRowOrder[aSwapIndex];
    aRowOrder[aSwapIndex] = aTemp;
  }

  const int aRowCount = std::min(4, kGridHeight);
  for (int aPickIndex = 0; aPickIndex < aRowCount; ++aPickIndex) {
    const int aRow = aRowOrder[aPickIndex];
    for (int aX = 0; aX < kGridWidth; ++aX) {
      MarkTileMatched(aX, aRow);
    }
  }
}

unsigned char GameBoard::GetRand(int pMax) {
  if (pMax <= 0 || mResultBufferLength == 0U) {
    return 0U;
  }

  const unsigned char aByte = mFastRand.Get(pMax);
  mRuntimeStats.mPasswordExpanderWraps = mFastRand.WrapCount();
  return aByte;
}

void GameBoard::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  Play();
}

unsigned char GameBoard::NextTypeByte() {
  return GetRand(256);
}

int GameBoard::SuccessfulMoveCount() const {
  return mSuccessfulMoveCount;
}

int GameBoard::BrokenCount() const {
  return mBrokenCount;
}

int GameBoard::TileTypeCount() const {
  return mTileTypeCount;
}

MatchBehavior GameBoard::GetMatchBehavior() const {
  return mMatchBehavior;
}

const GameTile* GameBoard::TileAt(int pGridX, int pGridY) const {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return nullptr;
  }
  return mGrid[pGridX][pGridY];
}

GameTile* GameBoard::MutableTileAt(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return nullptr;
  }
  return mGrid[pGridX][pGridY];
}

void GameBoard::RecordImpossible() {
  ++mRuntimeStats.mImpossible;
}

void GameBoard::RecordPlantedMatchSolve() {
  ++mRuntimeStats.mPlantedMatchSolve;
}

void GameBoard::RecordInconsistentStateD() {
  ++mRuntimeStats.mInconsistentStateD;
}

void GameBoard::RecordInconsistentStateE() {
  ++mRuntimeStats.mInconsistentStateE;
}

void GameBoard::RecordBlueMoonCase() {
  ++mRuntimeStats.mBlueMoonCase;
}

void GameBoard::RecordHarvestMoon() {
  ++mRuntimeStats.mHarvestMoon;
}

GameBoard::RuntimeStats GameBoard::GetRuntimeStats() const {
  return mRuntimeStats;
}

const char* GameBoard::GetName() const {
  return mGameName;
}

void GameBoard::SetGame(int pGameIndex) {
  ConfigureGameByIndex(pGameIndex);
}

void GameBoard::ConfigureGameByIndex(int pGameIndex) {
  GamePlayDirector* aDirector = nullptr;
  MoveBehavior aMoveBehavior = kSwap;
  MovePolicy aMovePolicy = kGreedy;
  MatchBehavior aMatchBehavior = kStreak;
  bool aIsCascading = true;
  int aTileTypeCount = 4;
  const char* aName = nullptr;

  switch (pGameIndex) {
    case IslandSwapFourGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aTileTypeCount = 4;
      aName = "Starlight Scramble [IS4G]";
      break;
    case IslandTapFourGreedy:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aIsCascading = true;
      aTileTypeCount = 4;
      aName = "Tea Time Tumble [IT4G]";
      break;
    case StreakSwapFourGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aTileTypeCount = 4;
      aName = "Knit Pickers [SS4G]";
      break;
    case StreakTapFourGreedy:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aIsCascading = true;
      aTileTypeCount = 4;
      aName = "King's Castle Quest [ST4G]";
      break;
    case IslandSwapFiveGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aTileTypeCount = 5;
      aName = "Relic Runes [IS5G]";
      break;
    case IslandTapFiveGreedy:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aIsCascading = true;
      aTileTypeCount = 5;
      aName = "Spooky Hallow [IT5G]";
      break;
    case StreakSwapFiveGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aTileTypeCount = 5;
      aName = "Totem Topple [SS5G]";
      break;
    case StreakTapFiveGreedy:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aIsCascading = true;
      aTileTypeCount = 5;
      aName = "Lady Bugs Blitz [ST5G]";
      break;
    case IslandSwapFourRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aTileTypeCount = 4;
      aName = "Glow Worm [IS4R]";
      break;
    case IslandTapFourRandom:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aIsCascading = true;
      aTileTypeCount = 4;
      aName = "Prism Ploppers [IT4R]";
      break;
    case StreakSwapFourRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aTileTypeCount = 4;
      aName = "Flux Fusion [SS4R]";
      break;
    case StreakTapFourRandom:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aIsCascading = true;
      aTileTypeCount = 4;
      aName = "Coral Crushers [ST4R]";
      break;
    case IslandSwapFiveRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aTileTypeCount = 5;
      aName = "Magma Meltdown [IS5R]";
      break;
    case IslandTapFiveRandom:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aIsCascading = true;
      aTileTypeCount = 5;
      aName = "Frost Fragment [IT5R]";
      break;
    case StreakSwapFiveRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aTileTypeCount = 5;
      aName = "Coffee Cubes [SS5R]";
      break;
    case StreakTapFiveRandom:
      aDirector = GamePlayDirector::TapStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aIsCascading = true;
      aTileTypeCount = 5;
      aName = "Wizard Smash [ST5R]";
      break;
    default:
      std::abort();
  }

  mGameIndex = pGameIndex;
  mTileTypeCount = aTileTypeCount;
  mGameName = aName;
  SetGamePlayDirector(aDirector);
  SetMoveBehavior(aMoveBehavior);
  SetMovePolicy(aMovePolicy);
  SetMatchBehavior(aMatchBehavior);
  SetIsCascading(aIsCascading);
}

GameTile* GameBoard::AcquireTile() {
  if (mTileStack == nullptr) {
    ++mBrokenCount;
    ++mRuntimeStats.mInconsistentStateA;
    RecordGameStateOverflowCatastrophic(kCatastropheCaseA);
    return nullptr;
  }

  GameTile* aTile = mTileStack;
  mTileStack = mTileStack->mNext;
  aTile->mNext = nullptr;
  return aTile;
}

void GameBoard::CheckSpecialEvents() {
  if (!HasPendingMatches()) {
    return;
  }

  if (GetRand(256) == 64U && GetRand(40) < 18U) {
    ++mRuntimeStats.mDragonAttack;
    DragonAttack();
  }

  if (GetRand(256) == 92U && GetRand(58) < 22U) {
    ++mRuntimeStats.mPhoenixAttack;
    PhoenixAttack();
  }

  if (GetRand(100) == 50U && GetRand(60) == 40U && GetRand(20) == 7U) {
    ++mRuntimeStats.mWyvernAttack;
    WyvernAttack();
  }
}

void GameBoard::ReleaseTile(GameTile* pTile) {
  if (pTile == nullptr) {
    return;
  }

  pTile->mPowerUpType = GamePowerUpType::kNone;
  pTile->mNext = mTileStack;
  mTileStack = pTile;
}

GameTile* GameBoard::GenerateTile(int pGridX, int pGridY) {
  if (!SeedCanDequeue()) {
    return nullptr;
  }

  GameTile* aTile = AcquireTile();
  if (aTile == nullptr) {
    return nullptr;
  }

  const unsigned char aTypeByte = NextTypeByte();
  const unsigned char aType = static_cast<unsigned char>(aTypeByte % static_cast<unsigned char>(mTileTypeCount));
  const unsigned char aByte = SeedDequeue();
  GamePowerUpType aPowerUpType = GamePowerUpType::kNone;
  if (aTypeByte < kPowerUpSpawnChance) {
    aPowerUpType = SelectPowerUpType(aByte);
    ++mRuntimeStats.mPowerUpSpawned;
  }
  aTile->Reset(pGridX, pGridY, aByte, aType, aPowerUpType);
  return aTile;
}

void GameBoard::ClearBoard() {
  for (int aX = 0; aX < kGridWidth; ++aX) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      if (mGrid[aX][aY] != nullptr) {
        ReleaseTile(mGrid[aX][aY]);
        mGrid[aX][aY] = nullptr;
      }
    }
  }
}

void GameBoard::ClearFrozenTiles() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = MutableTileAt(aX, aY);
      if (aTile != nullptr) {
        aTile->mFrozen = false;
      }
    }
  }
}

bool GameBoard::HasAnyTiles() const {
  for (int aX = 0; aX < kGridWidth; ++aX) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      if (mGrid[aX][aY] != nullptr) {
        return true;
      }
    }
  }
  return false;
}

int GameBoard::ActiveTileCount() const {
  int aCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mGrid[aX][aY] != nullptr) {
        ++aCount;
      }
    }
  }
  return aCount;
}

bool GameBoard::Empty() const {
  return !HasAnyTiles();
}

void GameBoard::InvalidateMatches() {
  MatchesBegin();
}

bool GameBoard::IsTileMatched(int pGridX, int pGridY) const {
  for (int aIndex = 0; aIndex < mMatchListCount; ++aIndex) {
    if (mMatchListX[aIndex] == pGridX && mMatchListY[aIndex] == pGridY) {
      return true;
    }
  }
  return false;
}

void GameBoard::MarkTileMatched(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return;
  }
  if (mGrid[pGridX][pGridY] == nullptr || IsTileMatched(pGridX, pGridY) || mMatchListCount >= kGridSize) {
    return;
  }
  mMatchListX[mMatchListCount] = pGridX;
  mMatchListY[mMatchListCount] = pGridY;
  ++mMatchListCount;
}

void GameBoard::ToppleStep() {
  mToppleSearchCount = 0;
  for (int aIndex = 0; aIndex < kGridSize; ++aIndex) {
    mToppleChanged[aIndex] = 0U;
    mToppleSearchMask[aIndex] = 0U;
  }

  auto MarkToppleChanged = [&](int pX, int pY) {
    if (pX < 0 || pX >= kGridWidth || pY < 0 || pY >= kGridHeight) {
      return;
    }
    mToppleChanged[pY * kGridWidth + pX] = 1U;
  };

  for (int aX = 0; aX < kGridWidth; ++aX) {
    int aWriteY = kGridHeight - 1;
    for (int aY = kGridHeight - 1; aY >= 0; --aY) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      if (aY != aWriteY) {
        MarkToppleChanged(aX, aY);
        MarkToppleChanged(aX, aWriteY);
        mGrid[aX][aWriteY] = aTile;
        mGrid[aX][aY] = nullptr;
        aTile->mGridX = aX;
        aTile->mGridY = aWriteY;
      }
      --aWriteY;
    }

    for (int aY = aWriteY; aY >= 0; --aY) {
      mGrid[aX][aY] = GenerateTile(aX, aY);
      MarkToppleChanged(aX, aY);
    }
  }

  for (int aFlat = 0; aFlat < kGridSize; ++aFlat) {
    if (mToppleChanged[aFlat] == 0U) {
      continue;
    }

    const int aX = aFlat % kGridWidth;
    const int aY = aFlat / kGridWidth;
    for (int aDY = -1; aDY <= 1; ++aDY) {
      for (int aDX = -1; aDX <= 1; ++aDX) {
        const int aNX = aX + aDX;
        const int aNY = aY + aDY;
        if (aNX < 0 || aNX >= kGridWidth || aNY < 0 || aNY >= kGridHeight) {
          continue;
        }

        const int aNeighborFlat = aNY * kGridWidth + aNX;
        if (mToppleSearchMask[aNeighborFlat] != 0U) {
          continue;
        }
        mToppleSearchMask[aNeighborFlat] = 1U;
        ++mToppleSearchCount;
      }
    }
  }
}

bool GameBoard::CascadeStep() {
  MatchesBegin();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      (void)MatchMark(aX, aY);
    }
  }
  return HasPendingMatches();
}

void GameBoard::Cascade() {
  (void)CascadeStep();
}

bool GameBoard::IsMatchStreak(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return false;
  }
  const GameTile* aCenter = mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return false;
  }
  const unsigned char aType = aCenter->mType;

  auto IsType = [&](int pX, int pY) -> bool {
    if (pX < 0 || pX >= kGridWidth || pY < 0 || pY >= kGridHeight) {
      return false;
    }
    const GameTile* aTile = mGrid[pX][pY];
    return (aTile != nullptr && aTile->mType == aType);
  };

  const bool aL = IsType(pGridX - 1, pGridY);
  if (aL && IsType(pGridX - 2, pGridY)) {
    return true;
  }
  const bool aR = IsType(pGridX + 1, pGridY);
  if (aR && IsType(pGridX + 2, pGridY)) {
    return true;
  }
  if (aL && aR) {
    return true;
  }

  const bool aU = IsType(pGridX, pGridY - 1);
  if (aU && IsType(pGridX, pGridY - 2)) {
    return true;
  }
  const bool aD = IsType(pGridX, pGridY + 1);
  if (aD && IsType(pGridX, pGridY + 2)) {
    return true;
  }
  return (aU && aD);
}

bool GameBoard::IsMatchIsland(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return false;
  }
  const GameTile* aCenter = mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return false;
  }
  const unsigned char aType = aCenter->mType;

  auto IsType = [&](int pX, int pY) -> bool {
    if (pX < 0 || pX >= kGridWidth || pY < 0 || pY >= kGridHeight) {
      return false;
    }
    const GameTile* aTile = mGrid[pX][pY];
    return (aTile != nullptr && aTile->mType == aType);
  };

  const bool aL = IsType(pGridX - 1, pGridY);
  if (aL && IsType(pGridX - 2, pGridY)) {
    return true;
  }
  const bool aR = IsType(pGridX + 1, pGridY);
  if (aR && IsType(pGridX + 2, pGridY)) {
    return true;
  }
  const bool aU = IsType(pGridX, pGridY - 1);
  if (aU && IsType(pGridX, pGridY - 2)) {
    return true;
  }
  const bool aD = IsType(pGridX, pGridY + 1);
  if (aD && IsType(pGridX, pGridY + 2)) {
    return true;
  }
  const bool aUL = IsType(pGridX - 1, pGridY - 1);
  if (aUL && (aL || aU)) {
    return true;
  }
  const bool aUR = IsType(pGridX + 1, pGridY - 1);
  if (aUR && (aR || aU)) {
    return true;
  }
  const bool aDL = IsType(pGridX - 1, pGridY + 1);
  if (aDL && (aD || aL)) {
    return true;
  }
  const bool aDR = IsType(pGridX + 1, pGridY + 1);
  if (aDR && (aD || aR)) {
    return true;
  }
  return false;
}

bool GameBoard::IsMatch(int pGridX, int pGridY) {
  return (mMatchBehavior == kIsland) ? IsMatchIsland(pGridX, pGridY) : IsMatchStreak(pGridX, pGridY);
}

bool GameBoard::MatchMarkStreak(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return false;
  }
  GameTile* aCenter = mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return false;
  }

  const unsigned char aType = aCenter->mType;
  bool aMatched = false;

  int aLeft = pGridX;
  while (aLeft > 0 && mGrid[aLeft - 1][pGridY] != nullptr && mGrid[aLeft - 1][pGridY]->mType == aType) {
    --aLeft;
  }
  int aRight = pGridX;
  while (aRight + 1 < kGridWidth && mGrid[aRight + 1][pGridY] != nullptr && mGrid[aRight + 1][pGridY]->mType == aType) {
    ++aRight;
  }
  if ((aRight - aLeft + 1) >= 3) {
    for (int aX = aLeft; aX <= aRight; ++aX) {
      if (mGrid[aX][pGridY] != nullptr) {
        MarkTileMatched(aX, pGridY);
      }
    }
    aMatched = true;
  }

  int aTop = pGridY;
  while (aTop > 0 && mGrid[pGridX][aTop - 1] != nullptr && mGrid[pGridX][aTop - 1]->mType == aType) {
    --aTop;
  }
  int aBottom = pGridY;
  while (aBottom + 1 < kGridHeight && mGrid[pGridX][aBottom + 1] != nullptr &&
         mGrid[pGridX][aBottom + 1]->mType == aType) {
    ++aBottom;
  }
  if ((aBottom - aTop + 1) >= 3) {
    for (int aY = aTop; aY <= aBottom; ++aY) {
      if (mGrid[pGridX][aY] != nullptr) {
        MarkTileMatched(pGridX, aY);
      }
    }
    aMatched = true;
  }
  return aMatched;
}

bool GameBoard::MatchMarkIsland(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return false;
  }
  GameTile* aCenter = mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return false;
  }

  int aStackSize = 0;
  int aComponentSize = 0;
  const unsigned char aType = aCenter->mType;
  for (int aIndex = 0; aIndex < kGridSize; ++aIndex) {
    mVisited[aIndex] = 0U;
  }

  mStackX[aStackSize] = pGridX;
  mStackY[aStackSize] = pGridY;
  ++aStackSize;
  mVisited[pGridY * kGridWidth + pGridX] = 1U;

  while (aStackSize > 0) {
    --aStackSize;
    const int aX = mStackX[aStackSize];
    const int aY = mStackY[aStackSize];
    mComponentX[aComponentSize] = aX;
    mComponentY[aComponentSize] = aY;
    ++aComponentSize;

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};
    for (int aDir = 0; aDir < 4; ++aDir) {
      if (aNX[aDir] < 0 || aNX[aDir] >= kGridWidth || aNY[aDir] < 0 || aNY[aDir] >= kGridHeight) {
        continue;
      }
      const int aNeighbor = aNY[aDir] * kGridWidth + aNX[aDir];
      if (mVisited[aNeighbor] != 0U) {
        continue;
      }
      GameTile* aTile = mGrid[aNX[aDir]][aNY[aDir]];
      if (aTile != nullptr && aTile->mType == aType) {
        mVisited[aNeighbor] = 1U;
        mStackX[aStackSize] = aNX[aDir];
        mStackY[aStackSize] = aNY[aDir];
        ++aStackSize;
      }
    }
  }

  if (aComponentSize < 3) {
    return false;
  }

  for (int aIndex = 0; aIndex < aComponentSize; ++aIndex) {
    GameTile* aTile = mGrid[mComponentX[aIndex]][mComponentY[aIndex]];
    if (aTile != nullptr) {
      MarkTileMatched(mComponentX[aIndex], mComponentY[aIndex]);
    }
  }
  return true;
}

bool GameBoard::MatchMark(int pGridX, int pGridY) {
  return (mMatchBehavior == kIsland) ? MatchMarkIsland(pGridX, pGridY) : MatchMarkStreak(pGridX, pGridY);
}

int GameBoard::GetMatches() {
  InvalidateMatches();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      (void)MatchMark(aX, aY);
    }
  }

  return mMatchListCount;
}

int GameBoard::CountMarkedTiles() const {
  return mMatchListCount;
}

int GameBoard::EvaluateCurrentMatches() {
  const int aCount = GetMatches();
  MatchesBegin();
  return aCount;
}

bool GameBoard::HasAnyMatches() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (IsMatch(aX, aY)) {
        return true;
      }
    }
  }
  return false;
}

bool GameBoard::HasAnyLegalMove() {
  return EnumerateMoves();
}

void GameBoard::MoveListClear() {
  mMoveListCount = 0;
}

void GameBoard::ResetMoveList() {
  mMoveListCount = 0;
}

bool GameBoard::PushMoveCandidate(int pX, int pY, bool pHorizontal, int pDir, int pScore) {
  if (mMoveListCount < 0 || mMoveListCount >= kMoveListCapacity) {
    return false;
  }
  mMoveListX[mMoveListCount] = pX;
  mMoveListY[mMoveListCount] = pY;
  mMoveListHorizontal[mMoveListCount] = pHorizontal;
  mMoveListDir[mMoveListCount] = pDir;
  mMoveListScore[mMoveListCount] = pScore;
  ++mMoveListCount;
  return true;
}

bool GameBoard::MoveListPush(int pX, int pY, bool pHorizontal, int pDir) {
  return PushMoveCandidate(pX, pY, pHorizontal, pDir, 0);
}

int GameBoard::SelectMoveIndex() {
  if (mMoveListCount <= 0) {
    return -1;
  }
  if (mMovePolicy == kGreedy) {
    int aBestIndex = 0;
    int aBestScore = mMoveListScore[0];
    for (int aIndex = 1; aIndex < mMoveListCount; ++aIndex) {
      if (mMoveListScore[aIndex] > aBestScore) {
        aBestScore = mMoveListScore[aIndex];
        aBestIndex = aIndex;
      }
    }
    return aBestIndex;
  }
  return static_cast<int>(GetRand(mMoveListCount));
}

void GameBoard::SwapTiles(int pX1, int pY1, int pX2, int pY2) {
  GameTile* aTileA = mGrid[pX1][pY1];
  GameTile* aTileB = mGrid[pX2][pY2];
  mGrid[pX1][pY1] = aTileB;
  mGrid[pX2][pY2] = aTileA;

  if (aTileA != nullptr) {
    aTileA->mGridX = pX2;
    aTileA->mGridY = pY2;
  }
  if (aTileB != nullptr) {
    aTileB->mGridX = pX1;
    aTileB->mGridY = pY1;
  }
}

void GameBoard::CollectSwapMoves() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mGrid[aX][aY] == nullptr) {
        continue;
      }

      if (aX + 1 < kGridWidth && mGrid[aX + 1][aY] != nullptr) {
        SwapTiles(aX, aY, aX + 1, aY);
        const int aScore = EvaluateCurrentMatches();
        if (aScore > 0) {
          (void)PushMoveCandidate(aX, aY, true, 1, aScore);
        }
        SwapTiles(aX, aY, aX + 1, aY);
      }

      if (aY + 1 < kGridHeight && mGrid[aX][aY + 1] != nullptr) {
        SwapTiles(aX, aY, aX, aY + 1);
        const int aScore = EvaluateCurrentMatches();
        if (aScore > 0) {
          (void)PushMoveCandidate(aX, aY, false, 1, aScore);
        }
        SwapTiles(aX, aY, aX, aY + 1);
      }
    }
  }
}

void GameBoard::CollectTapMoves() {
  for (int aIndex = 0; aIndex < kGridSize; ++aIndex) {
    mVisited[aIndex] = 0U;
  }

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      const int aFlat = aY * kGridWidth + aX;
      if (mVisited[aFlat] != 0U || mGrid[aX][aY] == nullptr) {
        continue;
      }

      MatchesBegin();
      (void)MatchMark(aX, aY);
      if (HasPendingMatches()) {
        const int aScore = CountMarkedTiles();
        (void)PushMoveCandidate(aX, aY, true, 0, aScore);
        for (int aMY = 0; aMY < kGridHeight; ++aMY) {
          for (int aMX = 0; aMX < kGridWidth; ++aMX) {
            GameTile* aTile = mGrid[aMX][aMY];
            if (aTile != nullptr && IsTileMatched(aMX, aMY)) {
              mVisited[aMY * kGridWidth + aMX] = 1U;
            }
          }
        }
      } else {
        mVisited[aFlat] = 1U;
      }
      MatchesBegin();
    }
  }
}

bool GameBoard::EnumerateMoves() {
  ResetMoveList();
  if (mMoveBehavior == kSwap) {
    CollectSwapMoves();
  } else {
    CollectTapMoves();
  }
  return (mMoveListCount > 0);
}

void GameBoard::ApplyMoveAt(int pMoveIndex) {
  if (pMoveIndex < 0 || pMoveIndex >= mMoveListCount) {
    return;
  }

  if (mMoveBehavior == kSwap) {
    const int aX = mMoveListX[pMoveIndex];
    const int aY = mMoveListY[pMoveIndex];
    const bool aHorizontal = mMoveListHorizontal[pMoveIndex];
    const int aOtherX = aHorizontal ? (aX + mMoveListDir[pMoveIndex]) : aX;
    const int aOtherY = aHorizontal ? aY : (aY + mMoveListDir[pMoveIndex]);
    SwapTiles(aX, aY, aOtherX, aOtherY);
    return;
  }

  MatchesBegin();
  (void)MatchMark(mMoveListX[pMoveIndex], mMoveListY[pMoveIndex]);
}

void GameBoard::ShuffleAllTiles() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      aTile->mType = static_cast<unsigned char>(GetRand(mTileTypeCount));
    }
  }
}

void GameBoard::ApocalypseScenario() {
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    return;
  }

  for (unsigned int aIndex = 0U; aIndex < mResultBufferLength; ++aIndex) {
    mResultBuffer[aIndex] = mSeedBuffer[aIndex % static_cast<unsigned int>(kSeedBufferCapacity)];
  }

  unsigned int aLeft = 0U;
  unsigned int aRight = mResultBufferLength - 1U;
  while (aLeft < aRight) {
    const unsigned char aTemp = mResultBuffer[aLeft];
    mResultBuffer[aLeft] = mResultBuffer[aRight];
    mResultBuffer[aRight] = aTemp;
    ++aLeft;
    --aRight;
  }

  mSeedReadIndex = mResultBufferLength;
  mResultBufferWriteIndex = 0U;
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mSeedBytesRemaining = 0U;
  mResultBufferWriteProgress = mResultBufferLength;
}

void GameBoard::ShuffleXY(int* pListX, int* pListY, int pCount) {
  if (pListX == nullptr || pListY == nullptr || pCount <= 1 || pCount > 255) {
    return;
  }

  for (int aIndex = pCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = static_cast<int>(GetRand(aIndex + 1));
    const int aTempX = pListX[aIndex];
    const int aTempY = pListY[aIndex];
    pListX[aIndex] = pListX[aSwapIndex];
    pListY[aIndex] = pListY[aSwapIndex];
    pListX[aSwapIndex] = aTempX;
    pListY[aSwapIndex] = aTempY;
  }
}

void GameBoard::MatchesBegin() {
  mMatchListCount = 0;
}

bool GameBoard::HasPendingMatches() const {
  return mMatchListCount > 0;
}

int GameBoard::MatchesCommit() {
  const int aMatchCount = mMatchListCount;
  GameTile* aMatchedTiles[kGridSize] = {};
  int aMatchedTileCount = 0;
  for (int aIndex = 0; aIndex < aMatchCount; ++aIndex) {
    const int aX = mMatchListX[aIndex];
    const int aY = mMatchListY[aIndex];
    if (aX < 0 || aX >= kGridWidth || aY < 0 || aY >= kGridHeight) {
      continue;
    }
    GameTile* const aTile = mGrid[aX][aY];
    if (aTile == nullptr) {
      continue;
    }
    aMatchedTiles[aMatchedTileCount] = aTile;
    ++aMatchedTileCount;
  }
  mMatchListCount = 0;

  for (int aIndex = aMatchedTileCount - 1; aIndex > 0; --aIndex) {
    const int aSwapIndex = static_cast<int>(GetRand(aIndex + 1));
    GameTile* const aTemp = aMatchedTiles[aIndex];
    aMatchedTiles[aIndex] = aMatchedTiles[aSwapIndex];
    aMatchedTiles[aSwapIndex] = aTemp;
  }

  int aCommitted = 0;
  for (int aIndex = 0; aIndex < aMatchedTileCount; ++aIndex) {
    GameTile* const aTile = aMatchedTiles[aIndex];
    if (aTile == nullptr) {
      continue;
    }
    const int aX = aTile->mGridX;
    const int aY = aTile->mGridY;
    if (aX < 0 || aX >= kGridWidth || aY < 0 || aY >= kGridHeight) {
      continue;
    }
    if (mGrid[aX][aY] != aTile) {
      continue;
    }
    EnqueueByte(aTile->mByte);
    mGrid[aX][aY] = nullptr;
    ReleaseTile(aTile);
    ++aCommitted;
  }
  return aCommitted;
}

bool GameBoard::TriggerMatchedPowerUps() {
  bool aTriggered = false;
  for (int aIndex = 0; aIndex < mMatchListCount; ++aIndex) {
      const int aX = mMatchListX[aIndex];
      const int aY = mMatchListY[aIndex];
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      if (aTile->mPowerUpType == GamePowerUpType::kNone) {
        continue;
      }

      const GamePowerUpType aPowerUpType = aTile->mPowerUpType;
      aTile->mPowerUpType = GamePowerUpType::kNone;
      if (aPowerUpType == GamePowerUpType::kZoneBomb) {
        gZoneBomb.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kRocket) {
        gRocket.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kColorBomb) {
        gColorBomb.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kCrossBomb) {
        gCrossBomb.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kPlasmaBeamH) {
        gPlasmaBeamH.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kPlasmaBeamV) {
        gPlasmaBeamV.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kPlasmaBeamQuad) {
        gPlasmaBeamQuad.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kNuke) {
        gNuke.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kCornerBomb) {
        gCornerBomb.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kVerticalBombs) {
        gVerticalBombs.Apply(this, aX, aY);
      } else if (aPowerUpType == GamePowerUpType::kHorizontalBombs) {
        gHorizontalBombs.Apply(this, aX, aY);
      }
      ++mRuntimeStats.mPowerUpConsumed;
      aTriggered = true;
  }
  return aTriggered;
}

int GameBoard::Match() {
  int aCommittedTotal = 0;
  if (!HasPendingMatches()) {
    (void)GetMatches();
  }
  int aResolutionLoop = 0;
  while (HasPendingMatches()) {
    CheckSpecialEvents();
    while (TriggerMatchedPowerUps()) {
    }
    aCommittedTotal += MatchesCommit();
    ++aResolutionLoop;
    if (aResolutionLoop > kVeryLargeEnsureIterations) {
      ++mBrokenCount;
      ++mRuntimeStats.mInconsistentStateL;
      RecordGameStateOverflowCatastrophic(kCatastropheCaseA);
      break;
    }
  }
  return aCommittedTotal;
}

void GameBoard::Topple() {
  ++mRuntimeStats.mTopple;
  ToppleStep();
  if (!ResolveBoardState_PostTopple()) {
    ++mBrokenCount;
    ++mRuntimeStats.mInconsistentStateC;
    RecordGameStateOverflowCatastrophic(kCatastropheCaseC);
  }
}

void GameBoard::Stuck() {
  ++mRuntimeStats.mStuck;
  MatchesBegin();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr) {
        MarkTileMatched(aX, aY);
      }
    }
  }
  (void)Match();
}

void GameBoard::Spawn() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mGrid[aX][aY] == nullptr) {
        mGrid[aX][aY] = GenerateTile(aX, aY);
      }
    }
  }

  if (!ResolveBoardState_PostSpawn()) {
    ++mBrokenCount;
    ++mRuntimeStats.mInconsistentStateB;
    RecordGameStateOverflowCatastrophic(kCatastropheCaseB);
  }
  InvalidateMatches();
}

bool GameBoard::MakeMoves() {
  if (!EnumerateMoves()) {
    if (mMoveBehavior == kTap) {
      if (!ResolveBoardState_PostSpawn() || !EnumerateMoves()) {
        return false;
      }
    } else {
      return false;
    }
  }

  const int aPick = SelectMoveIndex();
  if (aPick < 0) {
    return false;
  }
  ApplyMoveAt(aPick);
  return (mMoveBehavior == kTap) ? HasPendingMatches() : HasAnyMatches();
}

bool GameBoard::PlayMainLoop() {
  const bool aMoveApplied = MakeMoves();
  if (!aMoveApplied) {
    Stuck();
  } else {
    ++mSuccessfulMoveCount;
  }

  if (ActiveTileCount() < kGridSize) {
    if (HasAnyTiles()) {
      Stuck();
    }
    return true;
  }

  int aResolveLoopCount = 0;
  bool aFirstMatchInTurn = true;
  while (HasPendingMatches() || HasAnyMatches()) {
    const std::uint64_t aWriteCheckpoint = ResultWriteCheckpoint();
    ++aResolveLoopCount;
    if (aFirstMatchInTurn) {
      if (aMoveApplied) {
        ++mRuntimeStats.mUserMatch;
      }
      aFirstMatchInTurn = false;
    } else {
      ++mRuntimeStats.mCascadeMatch;
    }
    (void)Match();
    Topple();
    if (!mIsCascading) {
      break;
    }
    Cascade();

    if (!DidResultBufferMove(aWriteCheckpoint)) {
      break;
    }
    if (ActiveTileCount() < kGridSize) {
      if (HasAnyTiles()) {
        Stuck();
      }
      return true;
    }
    if (aResolveLoopCount > kVeryLargeEnsureIterations) {
      ++mBrokenCount;
      ++mRuntimeStats.mInconsistentStateM;
      RecordGameStateOverflowCatastrophic(kCatastropheCaseA);
      Stuck();
      break;
    }
  }

  if (Empty()) {
    Spawn();
  }
  return true;
}

void GameBoard::Play() {
  if (mResultBufferLength == 0U) {
    return;
  }

  mSeedReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mResultBufferWriteProgress = 0U;
  ShuffleSeedBuffer();
  Spawn();

  int aSuspendedLoop = 0;
  while (!IsResultBufferComplete() && aSuspendedLoop < kSuspendedThreshold) {
    const std::uint64_t aWriteCheckpoint = ResultWriteCheckpoint();

    int aLockedLoop = 0;
    while (aLockedLoop < kMaxLockedThreshold) {
      if (!PlayMainLoop()) {
        return;
      }
      if (DidResultBufferMove(aWriteCheckpoint)) {
        break;
      }
      ++aLockedLoop;
    }

    if (DidResultBufferMove(aWriteCheckpoint)) {
      aSuspendedLoop = 0;
      continue;
    }

    ++mRuntimeStats.mInconsistentStateN;
    RecordGameStateOverflowCataclysmic();
    ShuffleAllTiles();
    InvalidateMatches();
    ++aSuspendedLoop;
  }

  if (aSuspendedLoop >= kSuspendedThreshold) {
    ++mRuntimeStats.mInconsistentStateO;
    RecordGameStateOverflowApocalypse();
    ApocalypseScenario();
  }
}

}  // namespace peanutbutter::games
