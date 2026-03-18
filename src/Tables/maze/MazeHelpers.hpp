#ifndef BREAD_SRC_MAZE_MAZEHELPERS_HPP_
#define BREAD_SRC_MAZE_MAZEHELPERS_HPP_

#include "Maze.hpp"
#include "MazeCharacters.hpp"

namespace peanutbutter::maze::helpers {

constexpr int kPowerUpSpawnDivisor = 32;
constexpr int kMaxTrialTicks = 2048;

int ManhattanDistance(int pX1, int pY1, int pX2, int pY2);
int RandomDurationFromPick(int pPick);

template <typename TGetRandomWalkable, typename TOccupied>
inline bool PickUniqueWalkable(TGetRandomWalkable&& pGetRandomWalkable,
                               TOccupied&& pIsOccupied,
                               int* pOutX,
                               int* pOutY) {
  if (pOutX == nullptr || pOutY == nullptr) {
    return false;
  }

  for (int aAttempt = 0; aAttempt < 512; ++aAttempt) {
    int aX = -1;
    int aY = -1;
    if (!pGetRandomWalkable(aX, aY)) {
      return false;
    }
    if (pIsOccupied(aX, aY)) {
      continue;
    }
    *pOutX = aX;
    *pOutY = aY;
    return true;
  }

  return false;
}

template <typename TMaze, typename TNextIndex, typename TOccupied>
inline bool PickWalkableNearTarget(const TMaze& pMaze,
                                   TNextIndex&& pNextIndex,
                                   int pTargetX,
                                   int pTargetY,
                                   int pMaxDistance,
                                   TOccupied&& pIsOccupied,
                                   int* pOutX,
                                   int* pOutY) {
  if (pOutX == nullptr || pOutY == nullptr) {
    return false;
  }

  int aCandidateX[Maze::kGridSize];
  int aCandidateY[Maze::kGridSize];
  int aCandidateCount = 0;
  const int aMinY = (pTargetY - pMaxDistance > 0) ? (pTargetY - pMaxDistance) : 0;
  const int aMaxY =
      (pTargetY + pMaxDistance < Maze::kGridHeight - 1) ? (pTargetY + pMaxDistance) : (Maze::kGridHeight - 1);
  const int aMinX = (pTargetX - pMaxDistance > 0) ? (pTargetX - pMaxDistance) : 0;
  const int aMaxX =
      (pTargetX + pMaxDistance < Maze::kGridWidth - 1) ? (pTargetX + pMaxDistance) : (Maze::kGridWidth - 1);

  for (int aY = aMinY; aY <= aMaxY; ++aY) {
    for (int aX = aMinX; aX <= aMaxX; ++aX) {
      if (pMaze.IsWall(aX, aY) || pIsOccupied(aX, aY)) {
        continue;
      }
      if (ManhattanDistance(aX, aY, pTargetX, pTargetY) > pMaxDistance) {
        continue;
      }
      aCandidateX[aCandidateCount] = aX;
      aCandidateY[aCandidateCount] = aY;
      ++aCandidateCount;
    }
  }

  if (aCandidateCount <= 0) {
    return false;
  }

  const int aPick = pNextIndex(aCandidateCount);
  *pOutX = aCandidateX[aPick];
  *pOutY = aCandidateY[aPick];
  return true;
}

}  // namespace peanutbutter::maze::helpers

#endif  // BREAD_SRC_MAZE_MAZEHELPERS_HPP_
