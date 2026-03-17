#ifndef BREAD_TESTS_COMMON_COUNTERSEEDBUFFER_HPP_
#define BREAD_TESTS_COMMON_COUNTERSEEDBUFFER_HPP_

#include <vector>

namespace peanutbutter::tests {

template <typename TCounter>
std::vector<unsigned char> BuildCounterSeedBuffer(const unsigned char* pPassword, int pPasswordLength, int pOutputLength) {
  if (pOutputLength <= 0) {
    return {};
  }

  TCounter aCounter;
  aCounter.Seed(const_cast<unsigned char*>(pPassword), pPasswordLength);
  std::vector<unsigned char> aSeed(static_cast<std::size_t>(pOutputLength), 0U);
  aCounter.Get(aSeed.data(), pOutputLength);
  return aSeed;
}

}  // namespace peanutbutter::tests

#endif  // BREAD_TESTS_COMMON_COUNTERSEEDBUFFER_HPP_
