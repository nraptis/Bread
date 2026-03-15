#include "src/expansion/key_expansion/PasswordExpanderG.hpp"

#include <cstring>

#include "src/counters/MersenneCounter.hpp"

namespace bread::expansion::key_expansion {

void PasswordExpanderG::Expand(unsigned char* pPassword,
                               int pPasswordLength,
                               unsigned char* pExpanded) {
  CrashIfExpandedSizeInvalid("PasswordExpanderG");
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
  unsigned char aCarry = 0x5DU;
  for (int aRound = 0; aRound < PASSWORD_EXPANDED_SIZE; ++aRound) {
    const unsigned char a = mBuffer[aIndex];
    const unsigned char b = mWorkspace[aIndex];
    const unsigned char c = mWorkspace[(aIndex + 81) & kMask];
    const unsigned char x = RotL8(static_cast<unsigned char>(a ^ aCarry), b & 7U);
    aCarry = static_cast<unsigned char>(x + c + static_cast<unsigned char>(aRound));
    mBuffer[aIndex] = aCarry;
    aIndex = (aIndex + 409) & kMask;
  }

  std::memcpy(pExpanded, mBuffer, PASSWORD_EXPANDED_SIZE);
}

}  // namespace bread::expansion::key_expansion
