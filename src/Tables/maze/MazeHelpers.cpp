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

bool AnyActiveCheese(const std::array<CheeseState, kMaxCheeses>& pCheeses, int pCheeseCount) {
  for (int aIndex = 0; aIndex < pCheeseCount; ++aIndex) {
    if (pCheeses[aIndex].mActive) {
      return true;
    }
  }
  return false;
}

bool AnyPathingRobot(const std::array<RobotState, kMaxRobots>& pRobots, int pRobotCount) {
  for (int aIndex = 0; aIndex < pRobotCount; ++aIndex) {
    if (pRobots[aIndex].mAlive && !pRobots[aIndex].mVictorious) {
      return true;
    }
  }
  return false;
}

}  // namespace peanutbutter::maze::helpers
