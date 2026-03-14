#include "src/expansion/key_expansion/PasswordExpanderF.hpp"

#include <cstring>

#include "src/counters/ChaCha20Counter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderF::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  if (pExpanded == nullptr) {
    return;
  }

  PrimeWorkspace<ChaCha20Counter>(pPassword, pPasswordLength);

  constexpr int kMask = PASSWORD_EXPANDED_SIZE - 1;
  auto RotL8 = [](unsigned char pValue, unsigned int pShift) -> unsigned char {
    const unsigned int aShift = pShift & 7U;
    return static_cast<unsigned char>((static_cast<unsigned int>(pValue) << aShift) |
                                      (static_cast<unsigned int>(pValue) >> ((8U - aShift) & 7U)));
  };

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mWorkspace[aIndex];
    const unsigned char c = mBuffer[(aIndex + 114) & kMask];
    const unsigned char d = mWorkspace[(aIndex * 8 + 13) & kMask];
    const unsigned char t = static_cast<unsigned char>((a - b) ^ (c * d));
    mBuffer[aIndex] = RotL8(t, (b ^ d) & 7U);
  }

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; aIndex += 2) {
    const unsigned char aEven = mBuffer[aIndex];
    const unsigned char aOdd = mBuffer[aIndex + 1];
    mBuffer[aIndex] = static_cast<unsigned char>(aEven + RotL8(aOdd, 1U));
    mBuffer[aIndex + 1] = static_cast<unsigned char>(aOdd ^ RotL8(aEven, 3U));
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
