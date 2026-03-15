#ifndef BREAD_TESTS_COMMON_TESTS_HPP_
#define BREAD_TESTS_COMMON_TESTS_HPP_

#include "src/core/Bread.hpp"

namespace bread::tests::config {

// Master knobs for fast local iteration vs full suites.
inline constexpr int TEST_LOOP_COUNT = 10;
inline constexpr bool BENCHMARK_ENABLED = true;
inline constexpr bool REPEATABILITY_ENABLED = true;
inline constexpr bool GAME_REPEATABILITY_FULL_ENABLED = false;
inline constexpr int TEST_SEED_GLOBAL = 77;
inline constexpr int GAME_TEST_DATA_LENGTH = PASSWORD_EXPANDED_SIZE;
inline constexpr int MAZE_TEST_DATA_LENGTH = PASSWORD_EXPANDED_SIZE;

inline constexpr int ApplyGlobalSeed(int pLocalSeed) {
  return pLocalSeed + TEST_SEED_GLOBAL;
}

}  // namespace bread::tests::config

#endif  // BREAD_TESTS_COMMON_TESTS_HPP_
