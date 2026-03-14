#include "BreadCore.hpp"

namespace bread {

namespace {

std::uint8_t Mix(std::uint8_t pValue, std::uint8_t pSalt) {
  std::uint8_t aMixed = static_cast<std::uint8_t>(pValue ^ pSalt);
  aMixed = static_cast<std::uint8_t>((aMixed << 1) | (aMixed >> 7));
  aMixed = static_cast<std::uint8_t>(aMixed + static_cast<std::uint8_t>(pSalt * 17U));
  return aMixed;
}

}  // namespace

Glyph BreadCore::Seed(unsigned char* pPasswordBytes, Glyph pGlyph) {
  Glyph aGlyph = pGlyph;
  if (pPasswordBytes == nullptr) {
    return aGlyph;
  }

  for (std::size_t aY = 0; aY < 3; ++aY) {
    for (std::size_t aX = 0; aX < 3; ++aX) {
      const std::size_t aTileIndex = (aY * 3U) + aX;
      const std::size_t aPasswordOffset = aTileIndex % 8U;
      const std::uint8_t aByte = pPasswordBytes[aPasswordOffset];
      const std::uint8_t aMixed = Mix(aByte, static_cast<std::uint8_t>(aTileIndex));
      const bool aToggle = (aMixed & 0x01U) != 0;
      const bool aCurrent = aGlyph.Get(aX, aY) != 0U;
      const bool aNext = aToggle ? !aCurrent : aCurrent;
      aGlyph.Set(aX, aY, aNext);
    }
  }

  return aGlyph;
}

}  // namespace bread
