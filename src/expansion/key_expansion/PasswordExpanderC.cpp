#include "src/expansion/key_expansion/PasswordExpanderC.hpp"

#include <cstring>

#include "src/counters/MersenneCounter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderC::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  if (pExpanded == nullptr) {
    return;
  }

  PrimeWorkspace<MersenneCounter>(pPassword, pPasswordLength);

  constexpr int kMask = PASSWORD_EXPANDED_SIZE - 1;
  auto RotL8 = [](unsigned char pValue, unsigned int pShift) -> unsigned char {
    const unsigned int aShift = pShift & 7U;
    return static_cast<unsigned char>((static_cast<unsigned int>(pValue) << aShift) |
                                      (static_cast<unsigned int>(pValue) >> ((8U - aShift) & 7U)));
  };

  int aIndex = 0;
  unsigned char aPrev = 0xA5U;
  for (int aRound = 0; aRound < PASSWORD_EXPANDED_SIZE; ++aRound) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mWorkspace[aIndex];
    const unsigned char c = mWorkspace[(aIndex + 665) & kMask];
    const unsigned char x = RotL8(static_cast<unsigned char>(a ^ aPrev + 77), b & 7U);
    const unsigned char y = static_cast<unsigned char>(c + static_cast<unsigned char>(aRound));
    const unsigned char z = static_cast<unsigned char>(x ^ y);
    aPrev = static_cast<unsigned char>((z * a) ^ (b >> 1U));
    mBuffer[aIndex] = aPrev;
    aIndex = (aIndex + 1718) & kMask;
  }

  aPrev = mBuffer[kMask];
  for (int aIndex2 = 0; aIndex2 < PASSWORD_EXPANDED_SIZE; ++aIndex2) {
    const unsigned char aCur = mBuffer[aIndex2];
    const unsigned char w = mWorkspace[(aIndex2 * 9 + 11) & kMask];
    mBuffer[aIndex2] = RotL8(static_cast<unsigned char>(aCur ^ aPrev ^ w), (aCur >> 5U) & 7U);
    aPrev = aCur;
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
