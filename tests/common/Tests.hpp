#ifndef BREAD_TESTS_COMMON_TESTS_HPP_
#define BREAD_TESTS_COMMON_TESTS_HPP_

namespace bread::tests::config {

// Master knobs for fast local iteration vs full suites.
inline constexpr int TEST_LOOP_COUNT = 10000;
inline constexpr bool BENCHMARK_ENABLED = true;
inline constexpr bool REPEATABILITY_ENABLED = true;
inline constexpr bool GAME_REPEATABILITY_FULL_ENABLED = false;
inline constexpr int TEST_SEED_GLOBAL = 2;
inline constexpr int GAME_TEST_DATA_LENGTH = 9216 + 9216;
inline constexpr int MAZE_TEST_DATA_LENGTH = 9216 + 9216;

inline constexpr int ApplyGlobalSeed(int pLocalSeed) {
  return pLocalSeed + TEST_SEED_GLOBAL;
}

}  // namespace bread::tests::config

#endif  // BREAD_TESTS_COMMON_TESTS_HPP_
