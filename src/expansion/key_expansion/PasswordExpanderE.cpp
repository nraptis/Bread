#include "src/expansion/key_expansion/PasswordExpanderE.hpp"

#include <cstring>

#include "src/counters/AESCounter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderE::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  if (pExpanded == nullptr) {
    return;
  }

  PrimeWorkspace<AESCounter>(pPassword, pPasswordLength);

  constexpr int kMask = PASSWORD_EXPANDED_SIZE - 1;
  auto RotL8 = [](unsigned char pValue, unsigned int pShift) -> unsigned char {
    const unsigned int aShift = pShift & 7U;
    return static_cast<unsigned char>((static_cast<unsigned int>(pValue) << aShift) |
                                      (static_cast<unsigned int>(pValue) >> ((8U - aShift) & 7U)));
  };

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mWorkspace[aIndex];
    const unsigned char c = mWorkspace[(aIndex + 1033) & kMask];
    const unsigned char d = mBuffer[(aIndex - 32) & kMask];
    const unsigned char x = RotL8(static_cast<unsigned char>(a - c), b & 7U);
    const unsigned char y = RotL8(static_cast<unsigned char>(b * d), (c >> 5U) & 7U);
    mBuffer[aIndex] = static_cast<unsigned char>(((x * y) ^ c) + static_cast<unsigned char>(aIndex * 3));
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
