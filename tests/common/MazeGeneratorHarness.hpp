#ifndef BREAD_TESTS_COMMON_MAZEGENERATORHARNESS_HPP_
#define BREAD_TESTS_COMMON_MAZEGENERATORHARNESS_HPP_

#include <cstdint>

#include "src/Tables/maze/Maze.hpp"

namespace peanutbutter::tests::maze_generation {

class MazeGeneratorHarness final : public peanutbutter::maze::Maze {
 public:
  using peanutbutter::maze::Maze::Get;

  void Seed(unsigned char* pPassword, int pPasswordLength) override {
    InitializeSeedBuffer(pPassword, pPasswordLength);
    mIndexState = 0x811C9DC5U;
    if (pPassword == nullptr || pPasswordLength <= 0) {
      return;
    }
    for (int aIndex = 0; aIndex < pPasswordLength; ++aIndex) {
      mIndexState ^= static_cast<std::uint32_t>(pPassword[aIndex]);
      mIndexState *= 16777619U;
    }
  }

  void BuildPrims() {
    GeneratePrims();
    FinalizeWalls();
  }

  void BuildKruskals() {
    ExecuteKruskals();
    FinalizeWalls();
  }

 protected:
  int NextIndex(int pLimit) override {
    if (pLimit <= 1) {
      return 0;
    }
    mIndexState ^= (mIndexState << 13U);
    mIndexState ^= (mIndexState >> 17U);
    mIndexState ^= (mIndexState << 5U);
    return static_cast<int>(mIndexState % static_cast<std::uint32_t>(pLimit));
  }

 private:
  std::uint32_t mIndexState = 0x811C9DC5U;
};

}  // namespace peanutbutter::tests::maze_generation

#endif  // BREAD_TESTS_COMMON_MAZEGENERATORHARNESS_HPP_
