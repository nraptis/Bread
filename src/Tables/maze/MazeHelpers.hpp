#ifndef BREAD_SRC_MAZE_MAZEHELPERS_HPP_
#define BREAD_SRC_MAZE_MAZEHELPERS_HPP_

#include <array>

#include "Maze.hpp"

namespace peanutbutter::maze::helpers {

constexpr int kMaxRobots = 8;
constexpr int kMaxCheeses = 8;
constexpr int kMaxSharks = 8;
constexpr int kPowerUpSpawnDivisor = 32;
constexpr int kMaxTrialTicks = 2048;

enum class PowerUpType : unsigned char {
  kNone = 0,
  kInvincible = 1,
  kMagnet = 2,
  kSuperSpeed = 3,
  kTeleport = 4
};

struct RobotState {
  int mX = -1;
  int mY = -1;
  int mTargetCheese = -1;
  int mInvincibleHops = 0;
  int mMagnetHops = 0;
  int mSuperSpeedHops = 0;
  bool mAlive = false;
  bool mVictorious = false;
};

struct CheeseState {
  int mX = -1;
  int mY = -1;
  bool mActive = false;
};

struct SharkState {
  int mX = -1;
  int mY = -1;
  bool mActive = false;
};

int ManhattanDistance(int pX1, int pY1, int pX2, int pY2);
int RandomDurationFromPick(int pPick);
bool AnyActiveCheese(const std::array<CheeseState, kMaxCheeses>& pCheeses, int pCheeseCount);
bool AnyPathingRobot(const std::array<RobotState, kMaxRobots>& pRobots, int pRobotCount);

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

template <typename TNextIndex>
inline int ChooseActiveCheese(TNextIndex&& pNextIndex,
                              const std::array<CheeseState, kMaxCheeses>& pCheeses,
                              int pCheeseCount) {
  int aActive[kMaxCheeses];
  int aActiveCount = 0;
  for (int aIndex = 0; aIndex < pCheeseCount; ++aIndex) {
    if (!pCheeses[aIndex].mActive) {
      continue;
    }
    aActive[aActiveCount] = aIndex;
    ++aActiveCount;
  }
  if (aActiveCount <= 0) {
    return -1;
  }
  return aActive[pNextIndex(aActiveCount)];
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
