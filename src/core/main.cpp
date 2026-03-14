#include <cstddef>
#include <cstdint>
#include <iostream>

#include "Glyph.hpp"

int main() {
  bread::Glyph aGlyph;

  static_assert(sizeof(bread::Glyph) == 9, "Glyph must be exactly 9 bytes.");

  // Constructor should zero all bytes.
  for (std::size_t aY = 0; aY < 3; ++aY) {
    for (std::size_t aX = 0; aX < 3; ++aX) {
      if (aGlyph.Get(aX, aY) != 0U) {
        std::cerr << "Expected zero-initialized glyph.\n";
        return 1;
      }
    }
  }

  aGlyph.Set(0, 0, true);
  aGlyph.Set(1, 0, true);
  aGlyph.Set(2, 0, false);

  if (aGlyph.Get(0, 0) != 1U || aGlyph.Get(1, 0) != 1U || aGlyph.Get(2, 0) != 0U) {
    std::cerr << "Set/Get mismatch.\n";
    return 1;
  }

  // Coordinates are normalized with modulo 3.
  if (aGlyph.Get(3, 0) != aGlyph.Get(0, 0)) {
    std::cerr << "Coordinate normalization mismatch.\n";
    return 1;
  }

  std::cout << "Glyph API checks passed.\n";
  return 0;
}
