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
    const unsigned char b = mWorkspace[(aIndex * 5 + 17) & kMask];
    const unsigned char c = mBuffer[(aIndex + 97) & kMask];
    const unsigned char d = mWorkspace[(aIndex + 151) & kMask];
    const unsigned char x = static_cast<unsigned char>(a ^ static_cast<unsigned char>(b + c));
    const unsigned char y = static_cast<unsigned char>((b * 29U) ^ static_cast<unsigned char>(d * 17U));
    const unsigned char t = static_cast<unsigned char>(RotL8(x, d & 7U) + RotL8(c, 3U) + y);
    mBuffer[aIndex] = static_cast<unsigned char>(t ^ RotL8(static_cast<unsigned char>(a + d), (b >> 5U) & 7U));
  }

  for (int aRound = 0; aRound < 3; ++aRound) {
    for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
      const unsigned char a = mBuffer[aIndex];
      const unsigned char b = mBuffer[(aIndex + 1) & kMask];
      const unsigned char c = mBuffer[(aIndex + 65 + aRound * 11) & kMask];
      const unsigned char d = mWorkspace[(aIndex * 17 + aRound * 37) & kMask];
      const unsigned char z = static_cast<unsigned char>(a + b + static_cast<unsigned char>(c ^ d));
      mBuffer[aIndex] =
          static_cast<unsigned char>(RotL8(z, static_cast<unsigned int>((b ^ d ^ static_cast<unsigned char>(aRound)) & 7U)) ^
                                     RotL8(c, (a >> 3U) & 7U));
    }
  }

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; aIndex += 2) {
    const unsigned char aEven = mBuffer[aIndex];
    const unsigned char aOdd = mBuffer[aIndex + 1];
    mBuffer[aIndex] =
        static_cast<unsigned char>(aEven + RotL8(aOdd, 1U) + mWorkspace[(aIndex + 29) & kMask]);
    mBuffer[aIndex + 1] =
        static_cast<unsigned char>(aOdd ^ RotL8(aEven, 3U) ^ mWorkspace[(aIndex + 173) & kMask]);
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
