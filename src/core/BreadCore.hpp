#ifndef BREAD_BREAD_CORE_HPP_
#define BREAD_BREAD_CORE_HPP_

#include <cstddef>
#include <cstdint>

#include "Glyph.hpp"

namespace bread {

class BreadCore {
 public:
  // Caller must pass at least 8 password bytes.
  static Glyph Seed(unsigned char* pPasswordBytes, Glyph pGlyph);
};

}  // namespace bread

#endif  // BREAD_BREAD_CORE_HPP_
