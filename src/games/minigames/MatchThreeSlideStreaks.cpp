#include "src/games/minigames/MatchThreeSlideStreaks.hpp"

#include "src/games/engine/GamePlayDirector.hpp"

namespace bread::games {

MatchThreeSlideStreaks::MatchThreeSlideStreaks(bread::rng::Counter* pCounter)
    : GameBoard(pCounter, nullptr), GameBoard_Slide(), GameBoard_Streak() {
  SetPasswordExpander(&mOwnedPasswordExpander);
  SetGamePlayDirector(GamePlayDirector::BejeweledStyle());
}

void MatchThreeSlideStreaks::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  SetTypeCount(4);
  Play();
}

bool MatchThreeSlideStreaks::AttemptMove() {
  MoveListClear();

  for (int aRow = 0; aRow < kGridHeight; ++aRow) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideRow(aRow, aDir);
      if (HasAnyMatches()) {
        (void)MoveListPush(aRow, 0, true, aDir);
      }
      SlideRow(aRow, -aDir);
    }
  }

  for (int aCol = 0; aCol < kGridWidth; ++aCol) {
    for (int aAmount = 1; aAmount <= 7; ++aAmount) {
      const int aDir = -aAmount;
      SlideColumn(aCol, aDir);
      if (HasAnyMatches()) {
        (void)MoveListPush(aCol, 0, false, aDir);
      }
      SlideColumn(aCol, -aDir);
    }
  }

  const int aPick = MoveListPickIndex();
  if (aPick < 0) {
    return false;
  }

  if (mMoveListHorizontal[aPick]) {
    SlideRow(mMoveListX[aPick], mMoveListDir[aPick]);
  } else {
    SlideColumn(mMoveListX[aPick], mMoveListDir[aPick]);
  }
  return HasAnyMatches();
}

}  // namespace bread::games
