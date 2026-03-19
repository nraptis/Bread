#ifndef BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_
#define BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_

#include "Maze.hpp"

namespace peanutbutter::maze {

class MazeSpecialEvents final {
 public:
  static void StarBurst(Maze& pMaze,
                        helpers::MazeRobot* const* pRobots,
                        int pRobotCount);
  static void ChaosStorm(Maze& pMaze,
                         helpers::MazeShark* const* pSharks,
                         int pSharkCount);
  static void CometTrailsHorizontal(Maze& pMaze, const int* pRows, int pRowCount);
  static void CometTrailsVertical(Maze& pMaze, const int* pCols, int pColCount);

 private:
  static bool RepaintOrFlushTile(Maze& pMaze, int pX, int pY);
};

}  // namespace peanutbutter::maze

#endif  // BREAD_SRC_MAZE_MAZESPECIALEVENTS_HPP_
