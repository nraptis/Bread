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
      mGameIndex(StreakSwapGreedy),
      mGameName("StreakSwapGreedy"),
      mMoveListX{},
      mMoveListY{},
      mMoveListHorizontal{},
      mMoveListDir{},
      mMoveListScore{},
      mMoveListCount(0),
      mExploreListX{},
      mExploreListY{},
      mExploreListCount(0),
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
  ConfigureGameByIndex(StreakSwapGreedy);
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
  mMatchListCount = 0;
  mRuntimeStats = RuntimeStats{};

  InitializeSeedBuffer(pPassword, pPasswordLength);
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mFastRand.Seed(mSeedBuffer, pPasswordLength);
}

void GameBoard::SetMoveBehavior(MoveBehavior pMoveBehavior) {
  mMoveBehavior = pMoveBehavior;
  mIsCascading = (pMoveBehavior != kTap);
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

void GameBoard::RecordGameStateOverflowCatastrophic() {
  ++mRuntimeStats.mGameStateOverflowCatastrophic;
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

void GameBoard::RiddlerAttack() {
  for (int aRow = 0; aRow + 1 < kGridHeight; aRow += 2) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aA = mGrid[aX][aRow];
      GameTile* aB = mGrid[aX][aRow + 1];
      mGrid[aX][aRow] = aB;
      mGrid[aX][aRow + 1] = aA;
      if (aB != nullptr) {
        aB->mGridX = aX;
        aB->mGridY = aRow;
      }
      if (aA != nullptr) {
        aA->mGridX = aX;
        aA->mGridY = aRow + 1;
      }
    }
  }

  for (int aCol = 0; aCol + 1 < kGridWidth; aCol += 2) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      GameTile* aA = mGrid[aCol][aY];
      GameTile* aB = mGrid[aCol + 1][aY];
      mGrid[aCol][aY] = aB;
      mGrid[aCol + 1][aY] = aA;
      if (aB != nullptr) {
        aB->mGridX = aCol;
        aB->mGridY = aY;
      }
      if (aA != nullptr) {
        aA->mGridX = aCol + 1;
        aA->mGridY = aY;
      }
    }
  }

  InvalidateMatches();
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
  const char* aName = nullptr;

  switch (pGameIndex) {
    case StreakSwapGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aName = "StreakSwapGreedy";
      break;
    case StreakSlideGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aName = "StreakSlideGreedy";
      break;
    case IslandSwapGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aName = "IslandSwapGreedy";
      break;
    case IslandSlideGreedy:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aName = "IslandSlideGreedy";
      break;
    case StreakTapGreedy:
      aDirector = GamePlayDirector::CollapseStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kStreak;
      aName = "StreakTapGreedy";
      break;
    case IslandTapGreedy:
      aDirector = GamePlayDirector::CollapseStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kGreedy;
      aMatchBehavior = kIsland;
      aName = "IslandTapGreedy";
      break;
    case StreakSwapRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aName = "StreakSwapRandom";
      break;
    case StreakSlideRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aName = "StreakSlideRandom";
      break;
    case IslandSwapRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aName = "IslandSwapRandom";
      break;
    case IslandSlideRandom:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aName = "IslandSlideRandom";
      break;
    case StreakTapRandom:
      aDirector = GamePlayDirector::CollapseStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kStreak;
      aName = "StreakTapRandom";
      break;
    case IslandTapRandom:
      aDirector = GamePlayDirector::CollapseStyle();
      aMoveBehavior = kTap;
      aMovePolicy = kRandom;
      aMatchBehavior = kIsland;
      aName = "IslandTapRandom";
      break;
    case StreakSwapFirst:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kFirst;
      aMatchBehavior = kStreak;
      aName = "StreakSwapFirst";
      break;
    case StreakSlideFirst:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kFirst;
      aMatchBehavior = kStreak;
      aName = "StreakSlideFirst";
      break;
    case IslandSwapFirst:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSwap;
      aMovePolicy = kFirst;
      aMatchBehavior = kIsland;
      aName = "IslandSwapFirst";
      break;
    case IslandSlideFirst:
      aDirector = GamePlayDirector::MatchThreeStyle();
      aMoveBehavior = kSlide;
      aMovePolicy = kFirst;
      aMatchBehavior = kIsland;
      aName = "IslandSlideFirst";
      break;
    default:
      std::abort();
  }

  mGameIndex = pGameIndex;
  mGameName = aName;
  SetGamePlayDirector(aDirector);
  SetMoveBehavior(aMoveBehavior);
  SetMovePolicy(aMovePolicy);
  SetMatchBehavior(aMatchBehavior);
}

GameTile* GameBoard::AcquireTile() {
  if (mTileStack == nullptr) {
    ++mBrokenCount;
    RecordGameStateOverflowCatastrophic();
    return nullptr;
  }

  GameTile* aTile = mTileStack;
  mTileStack = mTileStack->mNext;
  aTile->mNext = nullptr;
  return aTile;
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
  const unsigned char aType = static_cast<unsigned char>(aTypeByte % static_cast<unsigned char>(kTypeCount));
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
  for (int aX = 0; aX < kGridWidth; ++aX) {
    int aWriteY = kGridHeight - 1;
    for (int aY = kGridHeight - 1; aY >= 0; --aY) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      if (aY != aWriteY) {
        mGrid[aX][aWriteY] = aTile;
        mGrid[aX][aY] = nullptr;
        aTile->mGridX = aX;
        aTile->mGridY = aWriteY;
      }
      --aWriteY;
    }

    for (int aY = aWriteY; aY >= 0; --aY) {
      mGrid[aX][aY] = GenerateTile(aX, aY);
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
  if (mMovePolicy == kFirst) {
    return 0;
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

void GameBoard::CollectSlideMoves() {
  for (int aRow = 0; aRow < kGridHeight; ++aRow) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideRow(aRow, aDir);
      const int aScore = EvaluateCurrentMatches();
      if (aScore > 0) {
        (void)PushMoveCandidate(aRow, 0, true, aDir, aScore);
      }
      SlideRow(aRow, -aDir);
    }
  }

  for (int aCol = 0; aCol < kGridWidth; ++aCol) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideColumn(aCol, aDir);
      const int aScore = EvaluateCurrentMatches();
      if (aScore > 0) {
        (void)PushMoveCandidate(aCol, 0, false, aDir, aScore);
      }
      SlideColumn(aCol, -aDir);
    }
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
  if (mMoveBehavior == kSlide) {
    CollectSlideMoves();
  } else if (mMoveBehavior == kSwap) {
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

  if (mMoveBehavior == kSlide) {
    if (mMoveListHorizontal[pMoveIndex]) {
      SlideRow(mMoveListX[pMoveIndex], mMoveListDir[pMoveIndex]);
    } else {
      SlideColumn(mMoveListX[pMoveIndex], mMoveListDir[pMoveIndex]);
    }
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
      aTile->mType = static_cast<unsigned char>(GetRand(kTypeCount));
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
  ShuffleXY(mMatchListX, mMatchListY, aMatchCount);

  int aCommitted = 0;
  for (int aIndex = 0; aIndex < aMatchCount; ++aIndex) {
    const int aX = mMatchListX[aIndex];
    const int aY = mMatchListY[aIndex];
    GameTile* aTile = mGrid[aX][aY];
    if (aTile == nullptr) {
      continue;
    }
    EnqueueByte(aTile->mByte);
    mGrid[aX][aY] = nullptr;
    ReleaseTile(aTile);
    ++aCommitted;
  }
  mMatchListCount = 0;
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

void GameBoard::Match() {
  if (!HasPendingMatches()) {
    (void)GetMatches();
  }
  if (HasPendingMatches()) {
    while (TriggerMatchedPowerUps()) {
    }
    (void)MatchesCommit();
  }
}

void GameBoard::SlideRow(int pRowIndex, int pAmount) {
  if (pRowIndex < 0 || pRowIndex >= kGridHeight || pAmount == 0) {
    return;
  }
  int aShift = pAmount % kGridWidth;
  if (aShift < 0) {
    aShift += kGridWidth;
  }
  if (aShift == 0) {
    return;
  }

  GameTile* aTemp[kGridWidth] = {};
  for (int aX = 0; aX < kGridWidth; ++aX) {
    aTemp[(aX + aShift) % kGridWidth] = mGrid[aX][pRowIndex];
  }
  for (int aX = 0; aX < kGridWidth; ++aX) {
    mGrid[aX][pRowIndex] = aTemp[aX];
    if (mGrid[aX][pRowIndex] != nullptr) {
      mGrid[aX][pRowIndex]->mGridX = aX;
      mGrid[aX][pRowIndex]->mGridY = pRowIndex;
    }
  }
}

void GameBoard::SlideColumn(int pColumnIndex, int pAmount) {
  if (pColumnIndex < 0 || pColumnIndex >= kGridWidth || pAmount == 0) {
    return;
  }
  int aShift = pAmount % kGridHeight;
  if (aShift < 0) {
    aShift += kGridHeight;
  }
  if (aShift == 0) {
    return;
  }

  GameTile* aTemp[kGridHeight] = {};
  for (int aY = 0; aY < kGridHeight; ++aY) {
    aTemp[(aY + aShift) % kGridHeight] = mGrid[pColumnIndex][aY];
  }
  for (int aY = 0; aY < kGridHeight; ++aY) {
    mGrid[pColumnIndex][aY] = aTemp[aY];
    if (mGrid[pColumnIndex][aY] != nullptr) {
      mGrid[pColumnIndex][aY]->mGridX = pColumnIndex;
      mGrid[pColumnIndex][aY]->mGridY = aY;
    }
  }
}

void GameBoard::Topple() {
  ++mRuntimeStats.mTopple;
  ToppleStep();
  if (!ResolveBoardState_PostTopple()) {
    ++mBrokenCount;
    RecordGameStateOverflowCatastrophic();
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
  Match();
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
    RecordGameStateOverflowCatastrophic();
  }
  InvalidateMatches();
}

bool GameBoard::MakeMoves() {
  if (!EnumerateMoves()) {
    return false;
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

  if (GetRand(256) == 86U && GetRand(128) < 16U) {
    ++mRuntimeStats.mRiddlerAttack;
    RiddlerAttack();
  }

  if ((GetRand(256) == 60U) && GetRand(128) < 16U) {
    ++mRuntimeStats.mDragonAttack;
    DragonAttack();
  }

  int aResolveLoopCount = 0;
  bool aFirstMatchInTurn = true;
  while (HasAnyMatches()) {
    const unsigned int aWriteProgressBefore = mResultBufferWriteProgress;
    ++aResolveLoopCount;
    if (aFirstMatchInTurn) {
      if (aMoveApplied) {
        ++mRuntimeStats.mUserMatch;
      }
      aFirstMatchInTurn = false;
    } else {
      ++mRuntimeStats.mCascadeMatch;
    }
    Match();
    Topple();
    Cascade();

    if (mResultBufferWriteProgress == aWriteProgressBefore) {
      break;
    }
    if (!mIsCascading) {
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
      RecordGameStateOverflowCatastrophic();
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
  while (mResultBufferWriteProgress < mResultBufferLength && aSuspendedLoop < kSuspendedThreshold) {
    mCataclysmWriteIndex = mResultBufferWriteIndex;

    int aLockedLoop = 0;
    while (aLockedLoop < kMaxLockedThreshold) {
      if (!PlayMainLoop()) {
        return;
      }
      if (mResultBufferWriteIndex != mCataclysmWriteIndex) {
        break;
      }
      ++aLockedLoop;
    }

    if (mResultBufferWriteIndex != mCataclysmWriteIndex) {
      aSuspendedLoop = 0;
      continue;
    }

    RecordGameStateOverflowCataclysmic();
    ShuffleAllTiles();
    InvalidateMatches();
    ++aSuspendedLoop;
  }

  if (aSuspendedLoop >= kSuspendedThreshold) {
    RecordGameStateOverflowApocalypse();
    ApocalypseScenario();
  }
}

}  // namespace peanutbutter::games
