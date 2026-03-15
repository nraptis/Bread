#ifndef BREAD_SRC_MAZE_MAZEROBOTCHEESE_HPP_
#define BREAD_SRC_MAZE_MAZEROBOTCHEESE_HPP_

#include "src/expansion/key_expansion/PasswordExpander.hpp"
#include "src/fast_rand/FastRand.hpp"
#include "src/maze/Maze.hpp"
#include "src/rng/Counter.hpp"

namespace bread::maze {

class MazeRobotCheese final : public Maze {
 public:
  static constexpr int kBufferCapacity = PASSWORD_EXPANDED_SIZE;

  MazeRobotCheese(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
                  bread::rng::Counter* pCounter);
  ~MazeRobotCheese() override = default;

  void Seed(unsigned char* pPassword, int pPasswordLength) override;

  int RobotX() const;
  int RobotY() const;
  int CheeseX() const;
  int CheeseY() const;

 private:
  int NextByte();
  int NextIndex(int pLimit);
  void FillStackAllCoords();
  void ShuffleStack();
  void SetInitialWalls();
  int CollectOpenComponents(bool pMarkLabels, int* pComponentLabel, int* pComponentCount, bool* pTouchesEdge);
  bool OpenWallForEnclosedComponent(int pComponentId, const int* pComponentLabel);
  bool OpenWallToMergeComponents(const int* pComponentLabel, int pComponentCount);
  void EnsureSimpleCornerPath();
  void Build();

  bread::rng::Counter* mCounter;
  bread::fast_rand::FastRand mFastRand;
  int mStackX[kGridSize];
  int mStackY[kGridSize];
  int mStackCount;
  int mRobotX;
  int mRobotY;
  int mCheeseX;
  int mCheeseY;
};

}  // namespace bread::maze

#endif  // BREAD_SRC_MAZE_MAZEROBOTCHEESE_HPP_
