#include <iostream>
#include <string_view>

#include "tests/common/Tests.hpp"
#include "tests/repeatability/RepeatabilitySuiteCore.hpp"

int main(int pArgc, char** pArgv) {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  const bool aRunFull = bread::tests::config::GAME_REPEATABILITY_FULL_ENABLED;
  const std::string_view aMode = (pArgc >= 2 && pArgv[1] != nullptr) ? pArgv[1] : "games";

  if (!bread::tests::repeatability::RunDigestRepeatabilitySuite(aMode, aLoops, aDataLength, aRunFull)) {
    std::cerr << "[FAIL] games repeatability suite failed\n";
    return 1;
  }

  return 0;
}
