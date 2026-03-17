#include "MazeHelpers.hpp"

namespace peanutbutter::maze::helpers {

int ManhattanDistance(int pX1, int pY1, int pX2, int pY2) {
  const int aDX = (pX1 > pX2) ? (pX1 - pX2) : (pX2 - pX1);
  const int aDY = (pY1 > pY2) ? (pY1 - pY2) : (pY2 - pY1);
  return aDX + aDY;
}

int RandomDurationFromPick(int pPick) {
  return 10 + (pPick % 7);
}

bool AnyValidCheese(const std::array<MazeCheese, kMaxCheeses>& pCheeses, int pCheeseCount) {
  for (int aIndex = 0; aIndex < pCheeseCount; ++aIndex) {
    if (pCheeses[aIndex].mIsValid) {
      return true;
    }
  }
  return false;
}

bool AnyRunnableRobot(const std::array<MazeRobot, kMaxRobots>& pRobots, int pRobotCount) {
  for (int aIndex = 0; aIndex < pRobotCount; ++aIndex) {
    if (!pRobots[aIndex].mDead || pRobots[aIndex].mVictorious) {
      return true;
    }
  }
  return false;
}

}  // namespace peanutbutter::maze::helpers
