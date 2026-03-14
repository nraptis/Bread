#include <array>
#include <iostream>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"

int main() {
  bread::expansion::key_expansion::PasswordExpanderA aExpander;
  std::array<unsigned char, 4> aPassword = {{1U, 3U, 3U, 7U}};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpanded = {};

  aExpander.Expand(aPassword.data(), static_cast<int>(aPassword.size()), aExpanded.data());
  int aNonZero = 0;
  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    if (aExpanded[aIndex] != 0U) {
      ++aNonZero;
    }
  }
  if (aNonZero == 0) {
    std::cerr << "[FAIL] password expander output unexpectedly zero\n";
    return 1;
  }

  std::cout << "[PASS] password expander test passed\n";
  return 0;
}
