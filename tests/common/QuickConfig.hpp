#ifndef BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_
#define BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_

#include "tests/common/Tests.hpp"

namespace peanutbutter::tests::common {

inline constexpr int kTinyLoops = peanutbutter::tests::config::TEST_LOOP_COUNT;
inline constexpr int kTinyBytes = peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;

}  // namespace peanutbutter::tests::common

#endif  // BREAD_TESTS_COMMON_QUICK_CONFIG_HPP_
