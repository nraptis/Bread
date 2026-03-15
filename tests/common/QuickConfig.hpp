#ifndef BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_
#define BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_

#include "tests/common/Tests.hpp"

namespace bread::tests::common {

inline constexpr int kTinyLoops = bread::tests::config::TEST_LOOP_COUNT;
inline constexpr int kTinyBytes = bread::tests::config::GAME_TEST_DATA_LENGTH;

}  // namespace bread::tests::common

#endif  // BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_
