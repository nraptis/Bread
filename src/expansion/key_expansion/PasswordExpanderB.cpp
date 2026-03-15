#include "src/expansion/key_expansion/PasswordExpanderB.hpp"

#include <cstring>

#include "src/counters/ChaCha20Counter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderB::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  CrashIfExpandedSizeInvalid("PasswordExpanderB");
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
    const unsigned char c = mWorkspace[(aIndex * 56 + 13) & kMask];
    const unsigned char d = mBuffer[(aIndex + 14) & kMask];

    const unsigned char x = RotL8(static_cast<unsigned char>(a + c), b & 7U);
    const unsigned char y = RotL8(static_cast<unsigned char>(b - d), c & 7U);
    mBuffer[aIndex] = static_cast<unsigned char>((x ^ y) + (a * b) + 0x3DU);
  }

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mBuffer[(aIndex + 112) & kMask];
    const unsigned char c = mWorkspace[(aIndex + 57) & kMask];
    mBuffer[aIndex] = RotL8(static_cast<unsigned char>(a ^ static_cast<unsigned char>(b + c)),
                            (aIndex >> 3U) & 7U);
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
