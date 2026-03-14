#include <array>
#include <iostream>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordExpanderA.hpp"

int main() {
  bread::expansion::key_expansion::PasswordExpanderA aExpander;
  std::array<unsigned char, 8> aPassword = {{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpanded = {};
  aExpander.Expand(aPassword.data(), static_cast<int>(aPassword.size()), aExpanded.data());
  std::cout << "[PASS] benchmark password expander quick run first=" << static_cast<int>(aExpanded[0]) << "\n";
  return 0;
}
