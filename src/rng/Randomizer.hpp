#ifndef BREAD_SRC_RNG_RANDOMIZER_HPP_
#define BREAD_SRC_RNG_RANDOMIZER_HPP_

#include "src/rng/Digest.hpp"

namespace bread::rng {

class Randomizer : public Digest {
 public:
  ~Randomizer() override = default;
};

}  // namespace bread::rng

#endif  // BREAD_SRC_RNG_RANDOMIZER_HPP_
