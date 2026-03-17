#include <iostream>
#include <string_view>

#include "tests/common/Tests.hpp"
#include "tests_regression/RepeatabilitySuiteCore.hpp"

int main(int pArgc, char** pArgv) {
  if (!peanutbutter::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] repeatability disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (peanutbutter::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;
  const bool aRunFull = peanutbutter::tests::config::GAME_REPEATABILITY_FULL_ENABLED;
  const std::string_view aMode = (pArgc >= 2 && pArgv[1] != nullptr) ? pArgv[1] : "digest";

  std::cout << "[INFO] digest repeatability mode=" << aMode << " loops=" << aLoops << " bytes=" << aDataLength
            << "\n";

  if (!peanutbutter::tests::repeatability::RunDigestRepeatabilitySuite(aMode, aLoops, aDataLength, aRunFull)) {
    std::cerr << "[FAIL] digest repeatability suite failed\n";
    return 1;
  }

  return 0;
}
