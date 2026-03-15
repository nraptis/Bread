#include <iostream>

#include "src/counters/MersenneCounter.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"
#include "src/maze/MazeRobotCheese.hpp"
#include "tests/common/MazeDisplay.hpp"

namespace {

struct SeedView {
  unsigned char* mBytes;
  int mLength;
};

}  // namespace

int main() {
  bread::expansion::key_expansion::PasswordExpanderA aExpander;
  MersenneCounter aCounter;
  bread::maze::MazeRobotCheese aMaze(&aExpander, &aCounter);

  unsigned char aSeed1[] = "display_robot_maze_a";
  unsigned char aSeed2[] = "william jennings bryant";
  unsigned char aSeed3[] = "who framed roger rabbir?";
  const SeedView aSeeds[] = {
      {aSeed1, static_cast<int>(sizeof(aSeed1) - 1)},
      {aSeed2, static_cast<int>(sizeof(aSeed2) - 1)},
      {aSeed3, static_cast<int>(sizeof(aSeed3) - 1)},
  };

  for (int aIndex = 0; aIndex < 3; ++aIndex) {
    aMaze.Seed(aSeeds[aIndex].mBytes, aSeeds[aIndex].mLength);
    std::cout << bread::tests::maze_display::BuildMazeString(aMaze, 'O');
    if (aIndex + 1 < 3) {
      std::cout << "\n.......\n";
    }
  }

  return 0;
}
