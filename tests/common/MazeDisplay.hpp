#ifndef BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_
#define BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_

#include <string>

#include "tests/common/MazeGeneratorHarness.hpp"

namespace peanutbutter::tests::maze_display {

inline std::string BuildMazeString(const peanutbutter::maze::Maze& pMaze,
                                   char pWallCharacter = 'x',
                                   char pWalkableCharacter = ' ') {
  std::string aResult;
  aResult.reserve(static_cast<std::size_t>(peanutbutter::maze::Maze::kGridHeight) *
                  static_cast<std::size_t>(peanutbutter::maze::Maze::kGridWidth + 3));

  for (int aY = 0; aY < peanutbutter::maze::Maze::kGridHeight; ++aY) {
    aResult.push_back('[');
    for (int aX = 0; aX < peanutbutter::maze::Maze::kGridWidth; ++aX) {
      aResult.push_back(pMaze.IsWall(aX, aY) ? pWallCharacter : pWalkableCharacter);
    }
    aResult.push_back(']');
    aResult.push_back('\n');
  }

  return aResult;
}

}  // namespace peanutbutter::tests::maze_display

#endif  // BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_
