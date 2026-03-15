#ifndef BREAD_SRC_MAZE_MAZEDOLPHINSHARKS_HPP_
#define BREAD_SRC_MAZE_MAZEDOLPHINSHARKS_HPP_

#include "src/expansion/key_expansion/PasswordExpander.hpp"
#include "src/fast_rand/FastRand.hpp"
#include "src/maze/Maze.hpp"
#include "src/rng/Counter.hpp"

namespace bread::maze {

class MazeDolphinSharks final : public Maze {
 public:
  static constexpr int kBufferCapacity = PASSWORD_EXPANDED_SIZE;

  MazeDolphinSharks(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander,
                    bread::rng::Counter* pCounter);
  ~MazeDolphinSharks() override = default;

  void Seed(unsigned char* pPassword, int pPasswordLength) override;

 private:
  int NextIndex(int pLimit);
  void FillStackAllCoords();
  void ShuffleStack();
  void SetInitialWalls();
  int CollectOpenComponents(bool pMarkLabels, int* pComponentLabel, int* pComponentCount, bool* pTouchesEdge);
  bool OpenWallForEnclosedComponent(int pComponentId, const int* pComponentLabel);
  bool OpenWallToMergeComponents(const int* pComponentLabel, int pComponentCount);
  void Build();

  bread::rng::Counter* mCounter;
  bread::fast_rand::FastRand mFastRand;
  int mStackX[kGridSize];
  int mStackY[kGridSize];
  int mStackCount;
};

}  // namespace bread::maze

#endif  // BREAD_SRC_MAZE_MAZEDOLPHINSHARKS_HPP_
