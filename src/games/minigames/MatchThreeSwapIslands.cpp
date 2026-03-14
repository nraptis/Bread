#include "src/games/minigames/MatchThreeSwapIslands.hpp"

namespace bread::games {

MatchThreeSwapIslands::MatchThreeSwapIslands(bread::rng::Counter* pCounter)
    : GameBoard(pCounter, kRequiredToNotExist, kIsland) {}

void MatchThreeSwapIslands::Seed(unsigned char* pPassword, int pPasswordLength) {
  InitializeSeed(pPassword, pPasswordLength);
  SetTypeCount(4);
  Play();
}

void MatchThreeSwapIslands::SwapTiles(int pX1, int pY1, int pX2, int pY2) {
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

bool MatchThreeSwapIslands::AttemptMove() {
  MoveListClear();

  for (int aY = 0; aY < kGridHeight; ++aY) {
    for (int aX = 0; aX < kGridWidth; ++aX) {
      if (mGrid[aX][aY] == nullptr) {
        continue;
      }

      if (aX + 1 < kGridWidth && mGrid[aX + 1][aY] != nullptr) {
        SwapTiles(aX, aY, aX + 1, aY);
        MatchesBegin();
        (void)MatchCheckIsland(aX, aY);
        (void)MatchCheckIsland(aX + 1, aY);
        if (HasPendingMatches()) {
          (void)MoveListPush(aX, aY, true, 1);
        }
        SwapTiles(aX, aY, aX + 1, aY);
      }

      if (aY + 1 < kGridHeight && mGrid[aX][aY + 1] != nullptr) {
        SwapTiles(aX, aY, aX, aY + 1);
        MatchesBegin();
        (void)MatchCheckIsland(aX, aY);
        (void)MatchCheckIsland(aX, aY + 1);
        if (HasPendingMatches()) {
          (void)MoveListPush(aX, aY, false, 1);
        }
        SwapTiles(aX, aY, aX, aY + 1);
      }
    }
  }

  const int aPick = MoveListPickIndex();
  if (aPick < 0) {
    MatchesBegin();
    return false;
  }

  const int aX = mMoveListX[aPick];
  const int aY = mMoveListY[aPick];
  const bool aHorizontal = mMoveListHorizontal[aPick];
  const int aOtherX = aHorizontal ? (aX + mMoveListDir[aPick]) : aX;
  const int aOtherY = aHorizontal ? aY : (aY + mMoveListDir[aPick]);

  SwapTiles(aX, aY, aOtherX, aOtherY);
  MatchesBegin();
  (void)MatchCheckIsland(aX, aY);
  (void)MatchCheckIsland(aOtherX, aOtherY);
  return HasPendingMatches();
}

}  // namespace bread::games
