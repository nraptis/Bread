#include "src/games/minigames/MatchThreeSlideIslands.hpp"

namespace bread::games {

MatchThreeSlideIslands::MatchThreeSlideIslands(bread::rng::Counter* pCounter)
    : GameBoard(pCounter, kRequiredToNotExist, kIsland) {}

void MatchThreeSlideIslands::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  SetTypeCount(4);
  Play();
}

bool MatchThreeSlideIslands::AttemptMove() {
  MoveListClear();

  for (int aRow = 0; aRow < kGridHeight; ++aRow) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideRow(aRow, aDir);
      MatchesBegin();
      for (int aX = 0; aX < kGridWidth; ++aX) {
        (void)MatchCheckIsland(aX, aRow);
      }
      if (HasPendingMatches()) {
        (void)MoveListPush(aRow, 0, true, aDir);
      }
      SlideRow(aRow, -aDir);
    }
  }

  for (int aCol = 0; aCol < kGridWidth; ++aCol) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideColumn(aCol, aDir);
      MatchesBegin();
      for (int aY = 0; aY < kGridHeight; ++aY) {
        (void)MatchCheckIsland(aCol, aY);
      }
      if (HasPendingMatches()) {
        (void)MoveListPush(aCol, 0, false, aDir);
      }
      SlideColumn(aCol, -aDir);
    }
  }

  const int aPick = MoveListPickIndex();
  if (aPick < 0) {
    MatchesBegin();
    return false;
  }

  if (mMoveListHorizontal[aPick]) {
    SlideRow(mMoveListX[aPick], mMoveListDir[aPick]);
    MatchesBegin();
    for (int aX = 0; aX < kGridWidth; ++aX) {
      (void)MatchCheckIsland(aX, mMoveListX[aPick]);
    }
  } else {
    SlideColumn(mMoveListX[aPick], mMoveListDir[aPick]);
    MatchesBegin();
    for (int aY = 0; aY < kGridHeight; ++aY) {
      (void)MatchCheckIsland(mMoveListX[aPick], aY);
    }
  }
  return HasPendingMatches();
}

}  // namespace bread::games
