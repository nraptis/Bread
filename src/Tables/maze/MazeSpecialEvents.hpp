#ifndef BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_
#define BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_

#include <array>

#include "Maze.hpp"
#include "MazeHelpers.hpp"

namespace peanutbutter::maze {

class MazeSpecialEvents final {
 public:
  static void StarBurst(Maze& pMaze,
                        const std::array<helpers::MazeRobot, helpers::kMaxRobots>& pRobots,
                        int pRobotCount);
  static void ChaosStorm(Maze& pMaze,
                         const std::array<helpers::MazeShark, helpers::kMaxSharks>& pSharks,
                         int pSharkCount);
  static void CometTrailsHorizontal(Maze& pMaze, const int* pRows, int pRowCount);
  static void CometTrailsVertical(Maze& pMaze, const int* pCols, int pColCount);

 private:
  static bool RepaintOrFlushTile(Maze& pMaze, int pX, int pY);
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_
