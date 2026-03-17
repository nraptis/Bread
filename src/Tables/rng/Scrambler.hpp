#ifndef BREAD_SRC_RNG_SCRAMBLER_HPP_
#define BREAD_SRC_RNG_SCRAMBLER_HPP_

namespace peanutbutter::rng {

class Scrambler {
 public:
  virtual ~Scrambler() = default;

  virtual void Get(unsigned char* pSource,
                   unsigned char* pWorker,
                   unsigned char* pDestination,
                   unsigned int pLength) = 0;
};

}  // namespace peanutbutter::rng

#endif  // BREAD_SRC_RNG_SCRAMBLER_HPP_
