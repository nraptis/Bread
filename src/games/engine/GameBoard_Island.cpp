#include "src/games/engine/GameBoard_Island.hpp"

namespace bread::games {

bool GameBoard_Island::MatchMark(int pGridX, int pGridY) {
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
    const int aX = mComponentX[aIndex];
    const int aY = mComponentY[aIndex];
    GameTile* aTile = mGrid[aX][aY];
    if (aTile != nullptr) {
      aTile->mIsMatched = true;
    }
  }
  mHasPendingMatches = true;
  return true;
}

bool GameBoard_Island::IsMatch(int pGridX, int pGridY) {
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

void GameBoard_Island::Match() {
  if (!HasPendingMatches()) {
    (void)GetMatches();
  }
  GameBoard::Match();
}

}  // namespace bread::games
