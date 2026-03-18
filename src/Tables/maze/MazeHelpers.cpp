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

}  // namespace peanutbutter::maze::helpers
