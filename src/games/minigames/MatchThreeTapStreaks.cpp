#include "src/games/minigames/MatchThreeTapStreaks.hpp"

namespace bread::games {

MatchThreeTapStreaks::MatchThreeTapStreaks(bread::rng::Counter* pCounter)
    : GameBoard(pCounter, kRequiredToExist, kStreak) {}

void MatchThreeTapStreaks::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  SetTypeCount(4);
  Play();
}

bool MatchThreeTapStreaks::AttemptMove() {
  MoveListClear();
  bool aVisited[kGridSize] = {};

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      const int aFlat = aY * kGridWidth + aX;
      if (aVisited[aFlat] || mGrid[aX][aY] == nullptr) {
        continue;
      }

      MatchesBegin();
      (void)MatchCheckStreak(aX, aY);
      if (HasPendingMatches()) {
        (void)MoveListPush(aX, aY, true, 0);
        for (int aMY = 0; aMY < kGridHeight; ++aMY) {
          for (int aMX = 0; aMX < kGridWidth; ++aMX) {
            GameTile* aTile = mGrid[aMX][aMY];
            if (aTile != nullptr && aTile->mIsMatched) {
              aVisited[aMY * kGridWidth + aMX] = true;
            }
          }
        }
      } else {
        aVisited[aFlat] = true;
      }
    }
  }

  const int aPick = MoveListPickIndex();
  if (aPick < 0) {
    MatchesBegin();
    return false;
  }

  MatchesBegin();
  (void)MatchCheckStreak(mMoveListX[aPick], mMoveListY[aPick]);
  return HasPendingMatches();
}

}  // namespace bread::games
