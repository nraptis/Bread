#ifndef BREAD_SRC_RNG_COUNTER_HPP_
#define BREAD_SRC_RNG_COUNTER_HPP_

#include "Digest.hpp"

namespace peanutbutter::rng {

class Counter : public Digest {
 public:
  ~Counter() override = default;

  using Digest::Get;
  virtual unsigned char Get() = 0;
};

}  // namespace peanutbutter::rng

#endif  // BREAD_SRC_RNG_COUNTER_HPP_
