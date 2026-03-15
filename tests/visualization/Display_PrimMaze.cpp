#include <iostream>

#include "tests/common/MazeDisplay.hpp"

int main() {
  bread::tests::maze_generation::MazeGeneratorHarness aMaze;
  unsigned char aSeed[] = "display_prim_maze";

  aMaze.Seed(aSeed, static_cast<int>(sizeof(aSeed) - 1));
  aMaze.BuildPrims();

  std::cout << bread::tests::maze_display::BuildMazeString(aMaze);
  return 0;
}
