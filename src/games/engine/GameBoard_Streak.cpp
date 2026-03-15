#include "src/games/engine/GameBoard_Streak.hpp"

namespace bread::games {

bool GameBoard_Streak::MatchMark(int pGridX, int pGridY) {
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
  while (aBottom + 1 < kGridHeight && mGrid[pGridX][aBottom + 1] != nullptr && mGrid[pGridX][aBottom + 1]->mType == aType) {
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

bool GameBoard_Streak::IsMatch(int pGridX, int pGridY) {
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

void GameBoard_Streak::Match() {
  if (!HasPendingMatches()) {
    (void)GetMatches();
  }
  GameBoard::Match();
}

}  // namespace bread::games
