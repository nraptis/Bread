#ifndef BREAD_SRC_MAZE_MAZECHARACTERS_HPP_
#define BREAD_SRC_MAZE_MAZECHARACTERS_HPP_

#include "MazeGrid.hpp"

namespace peanutbutter::maze::helpers {

constexpr int kMaxRobots = 8;
constexpr int kMaxCheeses = 8;
constexpr int kMaxSharks = 8;
constexpr int kMaxDolphins = 4;

class MazeRobot;
class MazeCheese;
class MazeShark;
class MazeDolphin;

using Robot = MazeRobot;
using Cheese = MazeCheese;
using Shark = MazeShark;
using Dolphin = MazeDolphin;

enum class PowerUpType : unsigned char { kNone = 0, kInvincible = 1, kMagnet = 2, kTeleport = 3 };

class MazeCheese {
 public:
  MazeCheese();

  void Reset();

  int mX;
  int mY;
};

class MazeRobot {
 public:
  MazeRobot();

  void Reset();
  bool AssignPathAndStartWalk(const int* pPathX, const int* pPathY, int pLength);
  void Update();
  void Die();

  Cheese* mCheese;
  int mX;
  int mY;
  int mPathX[peanutbutter::maze::MazeGrid::kGridSize];
  int mPathY[peanutbutter::maze::MazeGrid::kGridSize];
  int mPathLength;
  int mPathIndex;
  int mInvincibleTick;
  int mMagnetTick;
  int mTeleportTick;
  bool mInvincibleEnabled;
  bool mMagnetEnabled;
  bool mTeleportEnabled;
  bool mDeadFlag;
  bool mDidRecentlyRepath;
  bool mIsVictorious;
  bool mNeedsRepath;

 private:
  void ClearPath();
};

class MazeShark {
 public:
  MazeShark();

  void Reset();

  int mX;
  int mY;
  bool mDeadFlag;
  bool mDidRecentlyRevive;
};

class MazeDolphin {
 public:
  MazeDolphin();

  void Reset();

  int mX;
  int mY;
  bool mDeadFlag;
  bool mDidRecentlyRevive;
};

}  // namespace peanutbutter::maze::helpers

#endif  // BREAD_SRC_MAZE_MAZECHARACTERS_HPP_
