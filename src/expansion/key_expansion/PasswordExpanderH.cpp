#include "src/expansion/key_expansion/PasswordExpanderH.hpp"

#include <cstring>

#include "src/counters/ARIA256Counter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderH::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  CrashIfExpandedSizeInvalid("PasswordExpanderH");
  if (pExpanded == nullptr) {
    return;
  }

  PrimeWorkspace<ARIA256Counter>(pPassword, pPasswordLength);

  constexpr int kMask = PASSWORD_EXPANDED_SIZE - 1;
  auto RotL8 = [](unsigned char pValue, unsigned int pShift) -> unsigned char {
    const unsigned int aShift = pShift & 7U;
    return static_cast<unsigned char>((static_cast<unsigned int>(pValue) << aShift) |
                                      (static_cast<unsigned int>(pValue) >> ((8U - aShift) & 7U)));
  };
  auto SwapNibbles = [](unsigned char pValue) -> unsigned char {
    return static_cast<unsigned char>((pValue << 4U) | (pValue >> 4U));
  };

  for (int aIndex = PASSWORD_EXPANDED_SIZE - 1; aIndex >= 0; --aIndex) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mWorkspace[aIndex];
    const unsigned char c = mBuffer[(aIndex + 19) & kMask];
    const unsigned char d = mWorkspace[(aIndex - 8) & kMask];
    const unsigned char x = SwapNibbles(static_cast<unsigned char>(a + b));

    const unsigned char y = RotL8(static_cast<unsigned char>(c + d), b & 7U);
    mBuffer[aIndex] = static_cast<unsigned char>(x ^ y ^ static_cast<unsigned char>(aIndex));
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
