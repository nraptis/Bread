#include "src/games/engine/GameBoard.hpp"

#include <algorithm>
#include <cstring>

namespace bread::games {

namespace {
constexpr int kVeryLargeEnsureIterations = 1 << 20;
}

GameBoard::GameBoard(bread::rng::Counter* pCounter,
                     MatchRequiredMode pMatchRequiredMode,
                     MatchTypeMode pMatchType)
    : mGrid{},
      mCounter(pCounter),
      mTypeCount(4),
      mMatchRequiredMode(pMatchRequiredMode),
      mMatchType(pMatchType),
      mBuffer{},
      mBufferLength(0),
      mGetCursor(0),
      mPutCursor(0),
      mHasPendingMatches(false),
      mSuccessfulMoveCount(0),
      mBrokenCount(0),
      mMoveListX{},
      mMoveListY{},
      mMoveListHorizontal{},
      mMoveListDir{},
      mMoveListCount(0),
      mMatchCheckStack{},
      mMatchComponent{},
      mMatchVisited{} {
  std::memset(mGrid, 0, sizeof(mGrid));
  std::memset(mBuffer, 0, sizeof(mBuffer));
  std::memset(mMatchCheckStack, 0, sizeof(mMatchCheckStack));
  std::memset(mMatchComponent, 0, sizeof(mMatchComponent));
  std::memset(mMatchVisited, 0, sizeof(mMatchVisited));
  std::memset(mMoveListX, 0, sizeof(mMoveListX));
  std::memset(mMoveListY, 0, sizeof(mMoveListY));
  std::memset(mMoveListHorizontal, 0, sizeof(mMoveListHorizontal));
  std::memset(mMoveListDir, 0, sizeof(mMoveListDir));
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

  if (pPassword == nullptr || pPasswordLength <= 0) {
    pPasswordLength = 1;
    static unsigned char aFallback = 0U;
    pPassword = &aFallback;
  }

  mBufferLength = std::max(1, std::min(kSeedBufferCapacity, pPasswordLength));
  mGetCursor = 0;
  mPutCursor = 0;
  std::memset(mBuffer, 0, sizeof(mBuffer));
  std::memcpy(mBuffer, pPassword, static_cast<std::size_t>(mBufferLength));
}

void GameBoard::SetTypeCount(int pTypeCount) {
  if (pTypeCount <= 1) {
    mTypeCount = 2;
  } else {
    mTypeCount = pTypeCount;
  }
}

bool GameBoard::SeedCanDequeue() const {
  return (mGetCursor >= 0) && (mGetCursor < mBufferLength);
}

unsigned char GameBoard::SeedDequeue() {
  if (!SeedCanDequeue()) {
    return 0U;
  }
  const unsigned char aByte = mBuffer[mGetCursor];
  ++mGetCursor;
  return aByte;
}

void GameBoard::EnqueueByte(unsigned char pByte) {
  if (mBufferLength <= 0) {
    return;
  }

  mBuffer[mPutCursor] = pByte;
  ++mPutCursor;
  if (mPutCursor >= mBufferLength) {
    mPutCursor = 0;
  }
}

unsigned char GameBoard::NextTypeByte() {
  if (mCounter == nullptr) {
    return 0U;
  }
  return mCounter->Get();
}

void GameBoard::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }

  if (mBufferLength <= 0) {
    std::memset(pDestination, 0, static_cast<std::size_t>(pDestinationLength));
    return;
  }

  if (mGetCursor >= mBufferLength || mGetCursor < 0) {
    mGetCursor = 0;
  }

  int aOffset = 0;
  while (aOffset < pDestinationLength) {
    const int aToEnd = mBufferLength - mGetCursor;
    const int aTake = std::min(pDestinationLength - aOffset, aToEnd);
    std::memcpy(pDestination + aOffset, mBuffer + static_cast<std::ptrdiff_t>(mGetCursor),
                static_cast<std::size_t>(aTake));
    mGetCursor += aTake;
    aOffset += aTake;

    if (mGetCursor >= mBufferLength) {
      mGetCursor = 0;
    }
  }
}

unsigned char GameBoard::Get() {
  unsigned char aValue = 0U;
  Get(&aValue, 1);
  return aValue;
}

int GameBoard::SuccessfulMoveCount() const {
  return mSuccessfulMoveCount;
}

int GameBoard::BrokenCount() const {
  return mBrokenCount;
}

GameTile* GameBoard::GenerateTile(int pGridX, int pGridY) {
  if (!SeedCanDequeue()) {
    return nullptr;
  }
  const unsigned char aTypeSource = NextTypeByte();
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  const unsigned char aType = static_cast<unsigned char>(aTypeSource % static_cast<unsigned char>(aTypeCount));
  const unsigned char aByte = SeedDequeue();
  return new GameTile(pGridX, pGridY, aByte, aType);
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

bool GameBoard::IsMatchStreak(int pGridX, int pGridY) {
  return MatchCheckStreak(pGridX, pGridY);
}

bool GameBoard::IsMatchIsland(int pGridX, int pGridY) {
  return MatchCheckIsland(pGridX, pGridY);
}

bool GameBoard::HasAnyMatches() {
  InvalidateMatches();
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mMatchType == kIsland) {
        (void)IsMatchIsland(aX, aY);
      } else {
        (void)IsMatchStreak(aX, aY);
      }
      if (HasPendingMatches()) {
        return true;
      }
    }
  }
  return false;
}

void GameBoard::MoveListClear() {
  mMoveListCount = 0;
}

bool GameBoard::MoveListPush(int pX, int pY, bool pHorizontal, int pDir) {
  if (mMoveListCount < 0 || mMoveListCount >= static_cast<int>(8 * 8 * 4)) {
    return false;
  }
  mMoveListX[mMoveListCount] = pX;
  mMoveListY[mMoveListCount] = pY;
  mMoveListHorizontal[mMoveListCount] = pHorizontal;
  mMoveListDir[mMoveListCount] = pDir;
  ++mMoveListCount;
  return true;
}

int GameBoard::MoveListPickIndex() const {
  if (mMoveListCount <= 0) {
    return -1;
  }
  if (mCounter == nullptr) {
    return 0;
  }
  return static_cast<int>(mCounter->Get() % static_cast<unsigned char>(mMoveListCount));
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

bool GameBoard::MatchCheckStreak(int pGridX, int pGridY) {
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
  while (aRight + 1 < kGridWidth && mGrid[aRight + 1][pGridY] != nullptr &&
         mGrid[aRight + 1][pGridY]->mType == aType) {
    ++aRight;
  }
  if ((aRight - aLeft + 1) >= 3) {
    for (int aX = aLeft; aX <= aRight; ++aX) {
      GameTile* aTile = mGrid[aX][pGridY];
      if (aTile != nullptr) {
        aTile->mIsMatched = true;
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
      GameTile* aTile = mGrid[pGridX][aY];
      if (aTile != nullptr) {
        aTile->mIsMatched = true;
      }
    }
    aMatched = true;
  }

  if (aMatched) {
    mHasPendingMatches = true;
  }
  return aMatched;
}

bool GameBoard::MatchCheckIsland(int pGridX, int pGridY) {
  if (pGridX < 0 || pGridX >= kGridWidth || pGridY < 0 || pGridY >= kGridHeight) {
    return false;
  }
  GameTile* aCenter = mGrid[pGridX][pGridY];
  if (aCenter == nullptr) {
    return false;
  }

  std::memset(mMatchVisited, 0, sizeof(mMatchVisited));
  int aStackSize = 0;
  int aComponentSize = 0;
  const int aStartIndex = pGridY * kGridWidth + pGridX;
  const unsigned char aType = aCenter->mType;

  mMatchCheckStack[aStackSize++] = aStartIndex;
  mMatchVisited[aStartIndex] = true;

  while (aStackSize > 0) {
    const int aIndex = mMatchCheckStack[--aStackSize];
    mMatchComponent[aComponentSize++] = aIndex;

    const int aX = aIndex % kGridWidth;
    const int aY = aIndex / kGridWidth;

    const int aNX[4] = {aX - 1, aX + 1, aX, aX};
    const int aNY[4] = {aY, aY, aY - 1, aY + 1};

    for (int aDir = 0; aDir < 4; ++aDir) {
      if (aNX[aDir] < 0 || aNX[aDir] >= kGridWidth || aNY[aDir] < 0 || aNY[aDir] >= kGridHeight) {
        continue;
      }
      const int aNeighbor = aNY[aDir] * kGridWidth + aNX[aDir];
      if (mMatchVisited[aNeighbor]) {
        continue;
      }
      GameTile* aTile = mGrid[aNX[aDir]][aNY[aDir]];
      if (aTile != nullptr && aTile->mType == aType) {
        mMatchVisited[aNeighbor] = true;
        mMatchCheckStack[aStackSize++] = aNeighbor;
      }
    }
  }

  if (aComponentSize < 3) {
    return false;
  }

  for (int aIndex = 0; aIndex < aComponentSize; ++aIndex) {
    const int aFlat = mMatchComponent[aIndex];
    const int aX = aFlat % kGridWidth;
    const int aY = aFlat / kGridWidth;
    GameTile* aTile = mGrid[aX][aY];
    if (aTile != nullptr) {
      aTile->mIsMatched = true;
    }
  }
  mHasPendingMatches = true;
  return true;
}

int GameBoard::MatchesCommit() {
  int aCommitted = 0;
  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      GameTile* aTile = mGrid[aX][aY];
      if (aTile == nullptr || !aTile->mIsMatched) {
        continue;
      }
      EnqueueByte(aTile->mByte);
      mGrid[aX][aY] = nullptr;
      delete aTile;
      ++aCommitted;
    }
  }
  mHasPendingMatches = false;
  return aCommitted;
}

void GameBoard::Match() {
  if (HasPendingMatches()) {
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
      }
      --aWriteY;
    }

    for (int aY = aWriteY; aY >= 0; --aY) {
      mGrid[aX][aY] = GenerateTile(aX, aY);
      if (mGrid[aX][aY] != nullptr) {
        mGrid[aX][aY]->mIsNew = true;
      }
    }
  }

  if (mMatchRequiredMode == kRequiredToNotExist) {
    if (mMatchType == kIsland) {
      Topple_EnsureNoMatchesExistIsland();
    } else {
      Topple_EnsureNoMatchesExistStreak();
    }
  } else {
    if (mMatchType == kIsland) {
      Topple_EnsureMatchesExistIsland();
    } else {
      Topple_EnsureMatchesExistStreak();
    }
  }
  InvalidateNew();
}

void GameBoard::Stuck() {
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

  if (mMatchRequiredMode == kRequiredToNotExist) {
    if (mMatchType == kIsland) {
      Spawn_EnsureNoMatchesExistIsland();
    } else {
      Spawn_EnsureNoMatchesExistStreak();
    }
  }
  InvalidateNew();
}

void GameBoard::Spawn_EnsureNoMatchesExistStreak() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    bool aSuccess = true;
    InvalidateMatches();
    for (int aX = 0; aX < kGridWidth; ++aX) {
      for (int aY = 0; aY < kGridHeight; ++aY) {
        if (IsMatchStreak(aX, aY)) {
          GameTile* aTile = mGrid[aX][aY];
          if (aTile != nullptr) {
            aTile->mType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
            aSuccess = false;
          }
        }
      }
    }
    if (aSuccess) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

void GameBoard::Spawn_EnsureNoMatchesExistIsland() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    bool aSuccess = true;
    InvalidateMatches();
    for (int aX = 0; aX < kGridWidth; ++aX) {
      for (int aY = 0; aY < kGridHeight; ++aY) {
        if (IsMatchIsland(aX, aY)) {
          GameTile* aTile = mGrid[aX][aY];
          if (aTile != nullptr) {
            aTile->mType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
            aSuccess = false;
          }
        }
      }
    }
    if (aSuccess) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

void GameBoard::Topple_EnsureNoMatchesExistStreak() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    bool aSuccess = true;
    InvalidateMatches();
    for (int aX = 0; aX < kGridWidth; ++aX) {
      for (int aY = 0; aY < kGridHeight; ++aY) {
        GameTile* aTile = mGrid[aX][aY];
        if (aTile == nullptr || !aTile->mIsNew) {
          continue;
        }
        if (IsMatchStreak(aX, aY)) {
          aTile->mType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
          aSuccess = false;
        }
      }
    }
    if (aSuccess) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

void GameBoard::Topple_EnsureNoMatchesExistIsland() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    bool aSuccess = true;
    InvalidateMatches();
    for (int aX = 0; aX < kGridWidth; ++aX) {
      for (int aY = 0; aY < kGridHeight; ++aY) {
        GameTile* aTile = mGrid[aX][aY];
        if (aTile == nullptr || !aTile->mIsNew) {
          continue;
        }
        if (IsMatchIsland(aX, aY)) {
          aTile->mType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
          aSuccess = false;
        }
      }
    }
    if (aSuccess) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

void GameBoard::Topple_EnsureMatchesExistStreak() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    if (HasAnyMatches()) {
      aResolved = true;
      InvalidateMatches();
      break;
    }

    const unsigned char aType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
    bool aApplied = false;
    for (int aY = 0; aY < kGridHeight && !aApplied; ++aY) {
      for (int aX = 0; aX <= kGridWidth - 3; ++aX) {
        if (mGrid[aX][aY] != nullptr && mGrid[aX + 1][aY] != nullptr && mGrid[aX + 2][aY] != nullptr) {
          mGrid[aX][aY]->mType = aType;
          mGrid[aX + 1][aY]->mType = aType;
          mGrid[aX + 2][aY]->mType = aType;
          aApplied = true;
          break;
        }
      }
    }
    if (!aApplied) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

void GameBoard::Topple_EnsureMatchesExistIsland() {
  const int aTypeCount = (mTypeCount <= 0) ? 1 : mTypeCount;
  bool aResolved = false;

  for (int aLoop = 0; aLoop < kVeryLargeEnsureIterations; ++aLoop) {
    if (HasAnyMatches()) {
      aResolved = true;
      InvalidateMatches();
      break;
    }

    const unsigned char aType = static_cast<unsigned char>(NextTypeByte() % static_cast<unsigned char>(aTypeCount));
    bool aApplied = false;
    for (int aY = 0; aY < kGridHeight && !aApplied; ++aY) {
      for (int aX = 0; aX <= kGridWidth - 3; ++aX) {
        if (mGrid[aX][aY] != nullptr && mGrid[aX + 1][aY] != nullptr && mGrid[aX + 2][aY] != nullptr) {
          mGrid[aX][aY]->mType = aType;
          mGrid[aX + 1][aY]->mType = aType;
          mGrid[aX + 2][aY]->mType = aType;
          aApplied = true;
          break;
        }
      }
    }
    if (!aApplied) {
      aResolved = true;
      break;
    }
  }
  if (!aResolved) {
    ++mBrokenCount;
  }
  InvalidateMatches();
}

bool GameBoard::MakeMoves() {
  return AttemptMove();
}

void GameBoard::Play() {
  if (mBufferLength <= 0) {
    return;
  }

  mGetCursor = 0;
  mPutCursor = 0;
  Spawn();

  const int aMaxTurns = std::max(256, mBufferLength * 8);
  int aTurn = 0;
  int aNoGetProgressCount = 0;
  int aPrevGetCursor = mGetCursor;

  while (mGetCursor < mBufferLength && aTurn < aMaxTurns) {
    ++aTurn;

    // Moves mutate board state; matches are current board facts.
    const bool aMoveApplied = MakeMoves();
    if (!aMoveApplied) {
      Stuck();
    } else {
      ++mSuccessfulMoveCount;
      Match();
    }

    Topple();
    if (mGetCursor == aPrevGetCursor) {
      ++aNoGetProgressCount;
    } else {
      aNoGetProgressCount = 0;
      aPrevGetCursor = mGetCursor;
    }
    if (aNoGetProgressCount >= 128) {
      break;
    }
    if (Empty()) {
      break;
    }
  }

  if (!Empty()) {
    Stuck();
  }
}

}  // namespace bread::games
