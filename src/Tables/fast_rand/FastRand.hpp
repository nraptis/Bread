#ifndef BREAD_SRC_FAST_RAND_FASTRAND_HPP_
#define BREAD_SRC_FAST_RAND_FASTRAND_HPP_

#include <cstdint>

#include "../../PeanutButter.hpp"
#include "../password_expanders/ByteTwister.hpp"

namespace peanutbutter::fast_rand {

class FastRand {
 public:
  static constexpr unsigned int kSampleSlotCount = 16U;

  FastRand();

  void Seed(unsigned char* pPassword, int pPasswordLength);
  void Mix(unsigned char* pBuffer, int pBufferLength);

  unsigned char Get();
  unsigned char Get(int pMax);
  unsigned int GetInt();
  unsigned int NextRand(int pMax);
  std::uint64_t WrapCount() const;
  unsigned char CurrentTwistType() const;

 protected:
  void BuildSampleBuffer(unsigned char* pPassword, int pPasswordLength);
  unsigned char SelectTwistTypeFromBuffer(const unsigned char* pBuffer) const;
  void Wrap();

  unsigned char mSampleBuffer[PASSWORD_EXPANDED_SIZE];
  unsigned char mBufferTemp[PASSWORD_EXPANDED_SIZE];
  unsigned char mFastRandomBuffer[PASSWORD_EXPANDED_SIZE];
  unsigned int mFastRandomIndex;
  std::uint64_t mWrapCount;
  unsigned char mCurrentTwistType;
  unsigned char mSaltBuffer[twist::kSaltBytes];
  unsigned char mKeyStack[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes];
  unsigned char mNextRoundKeyBuffer[twist::kRoundKeyBytes];
};

}  // namespace peanutbutter::fast_rand

#endif  // BREAD_SRC_FAST_RAND_FASTRAND_HPP_
