#include <iostream>
#include <string_view>

#include "tests/common/Tests.hpp"
#include "tests/perseverance/PerseveranceSuiteCore.hpp"

int main(int pArgc, char** pArgv) {
  if (!bread::tests::config::REPEATABILITY_ENABLED) {
    std::cout << "[SKIP] perseverance disabled by Tests.hpp\n";
    return 0;
  }

  const int aLoops =
      (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  const std::string_view aMode = (pArgc >= 2 && pArgv[1] != nullptr) ? pArgv[1] : "games";

  std::cout << "[INFO] perseverance mode=" << aMode << " loops=" << aLoops << " bytes=" << aDataLength << "\n";

  if (!bread::tests::perseverance::RunDigestPerseveranceSuite(aMode, aLoops, aDataLength)) {
    std::cerr << "[FAIL] game perseverance suite failed\n";
    return 1;
  }

  return 0;
}
