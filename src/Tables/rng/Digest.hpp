#ifndef BREAD_SRC_RNG_DIGEST_HPP_
#define BREAD_SRC_RNG_DIGEST_HPP_

namespace peanutbutter::rng {

class Digest {
 public:
  virtual ~Digest() = default;

  virtual void Seed(unsigned char* pPassword, int pPasswordLength) = 0;
  virtual void Get(unsigned char* pDestination, int pDestinationLength) = 0;
};

}  // namespace peanutbutter::rng

#endif  // BREAD_SRC_RNG_DIGEST_HPP_
