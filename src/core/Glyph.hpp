#ifndef BREAD_GLYPH_HPP_
#define BREAD_GLYPH_HPP_

#include <cstddef>
#include <cstdint>

namespace bread {

class Glyph {
 public:
  Glyph();
  void Set(std::size_t pX, std::size_t pY, bool pValue);
  unsigned char Get(std::size_t pX, std::size_t pY) const;

 protected:
  unsigned char mData[9];

 private:
  static std::size_t NormalizeAxis(std::size_t pValue);
  static std::size_t FlattenIndex(std::size_t pX, std::size_t pY);
};

}  // namespace bread

#endif  // BREAD_GLYPH_HPP_
