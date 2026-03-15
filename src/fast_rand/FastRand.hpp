#ifndef BREAD_SRC_FAST_RAND_FASTRAND_HPP_
#define BREAD_SRC_FAST_RAND_FASTRAND_HPP_

#include <cstdint>

#include "src/expansion/key_expansion/PasswordExpander.hpp"

namespace bread::fast_rand {

enum class FastRandWrapMode {
  kWrapXOR,
  kWrapAdd
};

class FastRand {
 public:
  explicit FastRand(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander = nullptr,
                    FastRandWrapMode pWrapMode = FastRandWrapMode::kWrapXOR);

  void Seed(unsigned char* pPassword, int pPasswordLength);
  void SetSeedBuffer(const unsigned char* pSeed, int pSeedLength);
  void SetPasswordExpander(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander);
  void SetWrapMode(FastRandWrapMode pWrapMode);

  unsigned char Get();
  unsigned int GetInt();
  std::uint64_t WrapCount() const;

 protected:
  void Wrap();

  unsigned char mBufferSeed[PASSWORD_EXPANDED_SIZE];
  unsigned char mBufferTemp[PASSWORD_EXPANDED_SIZE];
  unsigned char mFastRandomBuffer[PASSWORD_EXPANDED_SIZE];
  FastRandWrapMode mWrapMode;
  unsigned int mFastRandomIndex;
  bread::expansion::key_expansion::PasswordExpander* mPasswordExpander;
  std::uint64_t mWrapCount;
  bool mHasCustomSeedBuffer;
};

}  // namespace bread::fast_rand

#endif  // BREAD_SRC_FAST_RAND_FASTRAND_HPP_
