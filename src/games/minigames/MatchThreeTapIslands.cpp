#include "src/games/minigames/MatchThreeTapIslands.hpp"

#include "src/games/engine/GamePlayDirector.hpp"

namespace bread::games {

MatchThreeTapIslands::MatchThreeTapIslands(bread::rng::Counter* pCounter)
    : GameBoard(pCounter, nullptr), GameBoard_Tap(), GameBoard_Island() {
  SetPasswordExpander(&mOwnedPasswordExpander);
  SetGamePlayDirector(GamePlayDirector::CollapseStyle());
}

void MatchThreeTapIslands::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  SetTypeCount(4);
  Play();
}

bool MatchThreeTapIslands::AttemptMove() {
  MoveListClear();
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
        (void)MoveListPush(aX, aY, true, 0);
        for (int aMY = 0; aMY < kGridHeight; ++aMY) {
          for (int aMX = 0; aMX < kGridWidth; ++aMX) {
            GameTile* aTile = mGrid[aMX][aMY];
            if (aTile != nullptr && aTile->mIsMatched) {
              mVisited[aMY * kGridWidth + aMX] = 1U;
            }
          }
        }
      } else {
        mVisited[aFlat] = 1U;
      }
    }
  }

  const int aPick = MoveListPickIndex();
  if (aPick < 0) {
    MatchesBegin();
    return false;
  }

  MatchesBegin();
  (void)MatchMark(mMoveListX[aPick], mMoveListY[aPick]);
  return HasPendingMatches();
}

}  // namespace bread::games
