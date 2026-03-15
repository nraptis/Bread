#include "src/expansion/key_expansion/PasswordExpanderD.hpp"

#include <cstring>

#include "src/counters/ARIA256Counter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderD::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  CrashIfExpandedSizeInvalid("PasswordExpanderD");
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
    const unsigned char c = mWorkspace[(aIndex - 116) & kMask];
    const unsigned char d = mBuffer[(aIndex + 119) & kMask];

    const unsigned char x = SwapNibbles(static_cast<unsigned char>(a * b));
    const unsigned char y = RotL8(static_cast<unsigned char>(c - d), (b >> 4U) & 7U);
    mBuffer[aIndex] = static_cast<unsigned char>(x ^ y ^ static_cast<unsigned char>(aIndex * 13));
  }

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; aIndex += 2) {
    const unsigned char aEven = mBuffer[aIndex];
    const unsigned char aOdd = mBuffer[aIndex + 1];
    const unsigned char wEven = mWorkspace[(aIndex * 13 + 90) & kMask];
    const unsigned char wOdd = mWorkspace[(aIndex * 11 + 87) & kMask];
    mBuffer[aIndex] = static_cast<unsigned char>(aEven + RotL8(aOdd, 1U) + wEven);
    mBuffer[aIndex + 1] = static_cast<unsigned char>(aOdd ^ RotL8(aEven, 3U) ^ wOdd);
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
