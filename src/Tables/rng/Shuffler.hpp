#ifndef BREAD_SRC_RNG_SHUFFLER_HPP_
#define BREAD_SRC_RNG_SHUFFLER_HPP_

#include <cstdint>

#include "../../PeanutButter.hpp"
#include "Digest.hpp"

namespace peanutbutter::rng {

class Shuffler : public Digest {
 public:
  static constexpr int kSeedBufferCapacity = PASSWORD_BALLOONED_SIZE;

  ~Shuffler() override = default;

  void Get(unsigned char* pDestination, int pDestinationLength) override;

 protected:
  Shuffler();

  void InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength);
  bool SeedCanDequeue() const;
  unsigned char SeedDequeue();
  void EnqueueByte(unsigned char pByte);

  unsigned char mSeedBuffer[kSeedBufferCapacity];
  unsigned char* mResultBuffer;
  unsigned int mSeedReadIndex;
  unsigned int mResultBufferWriteIndex;
  unsigned int mResultBufferLength;
  unsigned int mSeedBytesRemaining;
  std::uint64_t mResultBufferWriteProgress;
};

}  // namespace peanutbutter::rng

#endif  // BREAD_SRC_RNG_SHUFFLER_HPP_
