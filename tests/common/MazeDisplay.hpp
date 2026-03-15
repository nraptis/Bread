#ifndef BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_
#define BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_

#include <string>

#include "tests/common/MazeGeneratorHarness.hpp"

namespace bread::tests::maze_display {

inline std::string BuildMazeString(const bread::maze::Maze& pMaze,
                                   char pWallCharacter = 'x',
                                   char pWalkableCharacter = ' ') {
  std::string aResult;
  aResult.reserve(static_cast<std::size_t>(bread::maze::Maze::kGridHeight) *
                  static_cast<std::size_t>(bread::maze::Maze::kGridWidth + 3));

  for (int aY = 0; aY < bread::maze::Maze::kGridHeight; ++aY) {
    aResult.push_back('[');
    for (int aX = 0; aX < bread::maze::Maze::kGridWidth; ++aX) {
      aResult.push_back(pMaze.IsWall(aX, aY) ? pWallCharacter : pWalkableCharacter);
    }
    aResult.push_back(']');
    aResult.push_back('\n');
  }

  return aResult;
}

}  // namespace bread::tests::maze_display

#endif  // BREAD_TESTS_COMMON_MAZEDISPLAY_HPP_
