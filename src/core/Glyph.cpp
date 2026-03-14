#include "Glyph.hpp"

#include <cstring>

namespace bread {

Glyph::Glyph() {
  std::memset(mData, 0, sizeof(mData));
}

std::size_t Glyph::NormalizeAxis(std::size_t pValue) {
  return pValue % 3U;
}

std::size_t Glyph::FlattenIndex(std::size_t pX, std::size_t pY) {
  return (NormalizeAxis(pY) * 3U) + NormalizeAxis(pX);
}

void Glyph::Set(std::size_t pX, std::size_t pY, bool pValue) {
  mData[FlattenIndex(pX, pY)] = pValue ? 1U : 0U;
}

unsigned char Glyph::Get(std::size_t pX, std::size_t pY) const {
  return mData[FlattenIndex(pX, pY)] == 0U ? 0U : 1U;
}

}  // namespace bread
