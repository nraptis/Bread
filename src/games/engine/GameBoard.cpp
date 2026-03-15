#include "src/games/engine/GameBoard.hpp"

#include <algorithm>
#include <cstring>

#include "src/games/engine/GamePlayDirector.hpp"
#include "src/games/engine/GamePowerUp.hpp"

namespace bread::games {

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

GameBoard::GameBoard(bread::rng::Counter* pCounter,
                     bread::expansion::key_expansion::PasswordExpander* pPasswordExpander)
    : mGrid{},
      mCounter(pCounter),
      mFastRand(pPasswordExpander, bread::fast_rand::FastRandWrapMode::kWrapXOR),
      mGamePlayDirector(nullptr),
      mPlayTypeMode(kTap),
      mIsCascading(true),
      mSeedBuffer{},
      mResultBuffer(nullptr),
      mResultBufferWriteIndex(0U),
      mCataclysmWriteIndex(0U),
      mApocalypseWriteIndex(0U),
      mResultBufferWriteProgress(0U),
      mResultBufferReadIndex(0U),
      mResultBufferLength(0U),
      mSeedBytesRemaining(0U),
      mHasPendingMatches(false),
      mSuccessfulMoveCount(0),
      mBrokenCount(0),
      mMoveListX{},
      mMoveListY{},
      mMoveListHorizontal{},
      mMoveListDir{},
      mMoveListCount(0),
      mExploreListX{},
      mExploreListY{},
      mExploreListCount(0),
      mMatchListX{},
      mMatchListY{},
      mStackX{},
      mStackY{},
      mComponentX{},
      mComponentY{},
      mVisited{},
      mRuntimeStats{} {
  std::memset(mGrid, 0, sizeof(mGrid));
  std::memset(mSeedBuffer, 0, sizeof(mSeedBuffer));
}

GameBoard::~GameBoard() {
  ClearBoard();
}

void GameBoard::InitializeSeed(unsigned char* pPassword, int pPasswordLength) {
  ClearBoard();
  mHasPendingMatches = false;
  mSuccessfulMoveCount = 0;
  mBrokenCount = 0;
  mMoveListCount = 0;
  mExploreListCount = 0;
  mRuntimeStats = RuntimeStats{};

  if (pPassword == nullptr || pPasswordLength <= 0) {
    pPasswordLength = 1;
    static unsigned char aFallback = 0U;
    pPassword = &aFallback;
  }

  mResultBuffer = mSeedBuffer;
  mResultBufferLength = static_cast<unsigned int>(std::min(pPasswordLength, kSeedBufferCapacity));
  mResultBufferReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mResultBufferWriteProgress = 0U;
  mSeedBytesRemaining = mResultBufferLength;

  if (mCounter != nullptr) {
    mCounter->Seed(pPassword, pPasswordLength);
    mCounter->Get(mSeedBuffer, static_cast<int>(mResultBufferLength));
  } else {
    for (unsigned int aIndex = 0U; aIndex < mResultBufferLength; ++aIndex) {
      mSeedBuffer[aIndex] = pPassword[static_cast<std::size_t>(aIndex) % static_cast<std::size_t>(pPasswordLength)];
    }
  }
  mFastRand.SetSeedBuffer(mSeedBuffer, static_cast<int>(mResultBufferLength));
  mFastRand.Seed(pPassword, pPasswordLength);
}

void GameBoard::SetCounter(bread::rng::Counter* pCounter) {
  mCounter = pCounter;
}

void GameBoard::SetPasswordExpander(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander) {
  mFastRand.SetPasswordExpander(pPasswordExpander);
}

void GameBoard::SetTypeCount(int pTypeCount) {
  (void)pTypeCount;
}

void GameBoard::SetPlayTypeMode(PlayTypeMode pPlayTypeMode) {
  mPlayTypeMode = pPlayTypeMode;
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

bool GameBoard::MatchMark(int /*pGridX*/, int /*pGridY*/) {
  return false;
}

GameBoard_Slide::GameBoard_Slide() {
  SetPlayTypeMode(kSlide);
}

GameBoard_Swap::GameBoard_Swap() {
  SetPlayTypeMode(kSwap);
}

GameBoard_Tap::GameBoard_Tap() {
  SetPlayTypeMode(kTap);
  SetIsCascading(false);
}

bool GameBoard::SeedCanDequeue() const {
  return mResultBuffer != nullptr && mResultBufferLength > 0U && mSeedBytesRemaining > 0U;
}

unsigned char GameBoard::SeedDequeue() {
  if (!SeedCanDequeue()) {
    return 0U;
  }
  const unsigned char aByte = mResultBuffer[mResultBufferReadIndex];
  mResultBufferReadIndex = (mResultBufferReadIndex + 1U) % mResultBufferLength;
  --mSeedBytesRemaining;
  return aByte;
}

void GameBoard::EnqueueByte(unsigned char pByte) {
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    return;
  }
  mResultBuffer[mResultBufferWriteIndex] = pByte;
  mResultBufferWriteIndex = (mResultBufferWriteIndex + 1U) % mResultBufferLength;
  ++mResultBufferWriteProgress;
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
      if (aTile == nullptr || aTile->mIsMatched) {
        continue;
      }
      if (GetRand(3) == 0) {
        aTile->mIsMatched = true;
        aAny = true;
      }
    }
  }
  if (aAny) {
    mHasPendingMatches = true;
  }
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
  InvalidateNew();
}

unsigned char GameBoard::GetRand(int pMax) {
  if (pMax <= 0 || mResultBufferLength == 0U) {
    return 0U;
  }

  const unsigned char aByte = mFastRand.Get();
  mRuntimeStats.mPasswordExpanderWraps = mFastRand.WrapCount();
  return static_cast<unsigned char>(static_cast<int>(aByte) % pMax);
}

unsigned char GameBoard::NextTypeByte() {
  return GetRand(256);
}

void GameBoard::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    std::memset(pDestination, 0, static_cast<std::size_t>(pDestinationLength));
    return;
  }

  int aOffset = 0;
  while (aOffset < pDestinationLength) {
    const unsigned int aToEnd = mResultBufferLength - mResultBufferReadIndex;
    const int aTake = std::min(pDestinationLength - aOffset, static_cast<int>(aToEnd));
    std::memcpy(pDestination + aOffset, mResultBuffer + static_cast<std::ptrdiff_t>(mResultBufferReadIndex),
                static_cast<std::size_t>(aTake));
    mResultBufferReadIndex = (mResultBufferReadIndex + static_cast<unsigned int>(aTake)) % mResultBufferLength;
    aOffset += aTake;
  }
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

GameTile* GameBoard::GenerateTile(int pGridX, int pGridY) {
  if (!SeedCanDequeue()) {
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
  return new GameTile(pGridX, pGridY, aByte, aType, aPowerUpType);
}

void GameBoard::ClearBoard() {
  for (int aX = 0; aX < kGridWidth; ++aX) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      delete mGrid[aX][aY];
      mGrid[aX][aY] = nullptr;
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

void GameBoard::InvalidateNew() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr) {
        aTile->mIsNew = false;
      }
    }
  }
}

void GameBoard::InvalidateMatches() {
  MatchesBegin();
}

void GameBoard::InvalidateToppleFlags() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr) {
        aTile->mDidTopple = false;
      }
    }
  }
}

void GameBoard::ToppleStep() {
  InvalidateToppleFlags();

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
        aTile->mIsNew = false;
        aTile->mDidTopple = true;
      }
      --aWriteY;
    }

    for (int aY = aWriteY; aY >= 0; --aY) {
      mGrid[aX][aY] = GenerateTile(aX, aY);
      if (mGrid[aX][aY] != nullptr) {
        mGrid[aX][aY]->mIsNew = true;
        mGrid[aX][aY]->mDidTopple = true;
      }
    }
  }
}

bool GameBoard::CascadeStep() {
  MatchesBegin();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mDidTopple) {
        continue;
      }
      (void)MatchMark(aX, aY);
    }
  }
  return HasPendingMatches();
}

void GameBoard::Cascade() {
  (void)CascadeStep();
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
  if (mPlayTypeMode == kTap) {
    return HasAnyMatches();
  }

  if (mPlayTypeMode == kSwap) {
    for (int aY = 0; aY < kGridHeight; ++aY) {
      for (int aX = 0; aX < kGridWidth; ++aX) {
        if (mGrid[aX][aY] == nullptr) {
          continue;
        }

        if (aX + 1 < kGridWidth && mGrid[aX + 1][aY] != nullptr) {
          GameTile* aA = mGrid[aX][aY];
          GameTile* aB = mGrid[aX + 1][aY];
          mGrid[aX][aY] = aB;
          mGrid[aX + 1][aY] = aA;
          const bool aAny = HasAnyMatches();
          mGrid[aX][aY] = aA;
          mGrid[aX + 1][aY] = aB;
          if (aAny) {
            return true;
          }
        }

        if (aY + 1 < kGridHeight && mGrid[aX][aY + 1] != nullptr) {
          GameTile* aA = mGrid[aX][aY];
          GameTile* aB = mGrid[aX][aY + 1];
          mGrid[aX][aY] = aB;
          mGrid[aX][aY + 1] = aA;
          const bool aAny = HasAnyMatches();
          mGrid[aX][aY] = aA;
          mGrid[aX][aY + 1] = aB;
          if (aAny) {
            return true;
          }
        }
      }
    }
    return false;
  }

  if (mPlayTypeMode == kSlide) {
    for (int aRow = 0; aRow < kGridHeight; ++aRow) {
      for (int aAmount = 1; aAmount <= (kGridWidth - 1); ++aAmount) {
        const int aDir = -aAmount;
        SlideRow(aRow, aDir);
        const bool aAny = HasAnyMatches();
        SlideRow(aRow, -aDir);
        if (aAny) {
          return true;
        }
      }
    }
    for (int aCol = 0; aCol < kGridWidth; ++aCol) {
      for (int aAmount = 1; aAmount <= (kGridHeight - 1); ++aAmount) {
        const int aDir = -aAmount;
        SlideColumn(aCol, aDir);
        const bool aAny = HasAnyMatches();
        SlideColumn(aCol, -aDir);
        if (aAny) {
          return true;
        }
      }
    }
    return false;
  }

  return false;
}

bool GameBoard::IsMatch(int /*pGridX*/, int /*pGridY*/) {
  return false;
}

int GameBoard::GetMatches() {
  InvalidateMatches();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      (void)MatchMark(aX, aY);
    }
  }

  int aCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      const GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr && aTile->mIsMatched) {
        ++aCount;
      }
    }
  }
  return aCount;
}

void GameBoard::MoveListClear() {
  mMoveListCount = 0;
}

bool GameBoard::MoveListPush(int pX, int pY, bool pHorizontal, int pDir) {
  if (mMoveListCount < 0 || mMoveListCount >= kMoveListCapacity) {
    return false;
  }
  mMoveListX[mMoveListCount] = pX;
  mMoveListY[mMoveListCount] = pY;
  mMoveListHorizontal[mMoveListCount] = pHorizontal;
  mMoveListDir[mMoveListCount] = pDir;
  ++mMoveListCount;
  return true;
}

int GameBoard::MoveListPickIndex() {
  if (mMoveListCount <= 0) {
    return -1;
  }
  return static_cast<int>(GetRand(mMoveListCount));
}

void GameBoard::ShuffleAllTiles() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr) {
        continue;
      }
      aTile->mType = static_cast<unsigned char>(GetRand(kTypeCount));
      aTile->mIsNew = false;
      aTile->mDidTopple = false;
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

  mResultBufferReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mResultBufferWriteProgress = 0U;
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
  mHasPendingMatches = false;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr) {
        aTile->mIsMatched = false;
      }
    }
  }
}

bool GameBoard::HasPendingMatches() const {
  return mHasPendingMatches;
}

int GameBoard::MatchesCommit() {
  int aMatchCount = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mIsMatched) {
        continue;
      }
      if (aMatchCount >= kGridSize) {
        continue;
      }
      mMatchListX[aMatchCount] = aX;
      mMatchListY[aMatchCount] = aY;
      ++aMatchCount;
    }
  }

  ShuffleXY(mMatchListX, mMatchListY, aMatchCount);

  int aCommitted = 0;
  for (int aIndex = 0; aIndex < aMatchCount; ++aIndex) {
    const int aX = mMatchListX[aIndex];
    const int aY = mMatchListY[aIndex];
    GameTile* aTile = mGrid[aX][aY];
    if (aTile == nullptr || !aTile->mIsMatched) {
      continue;
    }
    EnqueueByte(aTile->mByte);
    mGrid[aX][aY] = nullptr;
    delete aTile;
    ++aCommitted;
  }
  mHasPendingMatches = false;
  return aCommitted;
}

bool GameBoard::TriggerMatchedPowerUps() {
  bool aTriggered = false;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mIsMatched) {
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
  }
  return aTriggered;
}

void GameBoard::Match() {
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
  InvalidateNew();
}

void GameBoard::Stuck() {
  ++mRuntimeStats.mStuck;
  MatchesBegin();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile != nullptr) {
        aTile->mIsMatched = true;
      }
    }
  }
  mHasPendingMatches = true;
  Match();
}

void GameBoard::Spawn() {
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mGrid[aX][aY] == nullptr) {
        mGrid[aX][aY] = GenerateTile(aX, aY);
        if (mGrid[aX][aY] != nullptr) {
          mGrid[aX][aY]->mIsNew = true;
        }
      }
    }
  }

  if (!ResolveBoardState_PostSpawn()) {
    ++mBrokenCount;
    RecordGameStateOverflowCatastrophic();
  }
  InvalidateMatches();
  InvalidateNew();
}

bool GameBoard::MakeMoves() {
  return AttemptMove();
}

bool GameBoard::PlayMainLoop() {
  const bool aMoveApplied = MakeMoves();
  if (!aMoveApplied) {
    Stuck();
  } else {
    ++mSuccessfulMoveCount;
  }

  if (ActiveTileCount() < kGridSize) {
    Stuck();
    return false;
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
      Stuck();
      return false;
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

  mResultBufferReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mCataclysmWriteIndex = 0U;
  mApocalypseWriteIndex = 0U;
  mResultBufferWriteProgress = 0U;
  ShuffleSeedBuffer();
  mFastRand.SetSeedBuffer(mSeedBuffer, static_cast<int>(mResultBufferLength));
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
    InvalidateNew();
    ++aSuspendedLoop;
  }

  if (aSuspendedLoop >= kSuspendedThreshold) {
    RecordGameStateOverflowApocalypse();
    ApocalypseScenario();
    return;
  }
}

}  // namespace bread::games
