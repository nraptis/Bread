#include "src/fast_rand/FastRand.hpp"

#include <cstring>

namespace bread::fast_rand {

namespace {

void FillRepeatedBuffer(unsigned char* pDestination, const unsigned char* pSource, int pSourceLength) {
  if (pDestination == nullptr) {
    return;
  }
  if (pSource == nullptr || pSourceLength <= 0) {
    std::memset(pDestination, 0, PASSWORD_EXPANDED_SIZE);
    return;
  }

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
    pDestination[aIndex] = pSource[aIndex % pSourceLength];
  }
}

}  // namespace

FastRand::FastRand(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander, FastRandWrapMode pWrapMode)
    : mBufferSeed{},
      mBufferTemp{},
      mFastRandomBuffer{},
      mWrapMode(pWrapMode),
      mFastRandomIndex(0U),
      mPasswordExpander(pPasswordExpander),
      mWrapCount(0U),
      mHasCustomSeedBuffer(false) {}

void FastRand::Seed(unsigned char* pPassword, int pPasswordLength) {
  if (pPassword == nullptr || pPasswordLength <= 0) {
    static unsigned char aFallback = 0U;
    pPassword = &aFallback;
    pPasswordLength = 1;
  }

  if (!mHasCustomSeedBuffer) {
    FillRepeatedBuffer(mBufferSeed, pPassword, pPasswordLength);
  }

  if (mPasswordExpander != nullptr) {
    mPasswordExpander->Expand(pPassword, pPasswordLength, mFastRandomBuffer);
  } else {
    FillRepeatedBuffer(mFastRandomBuffer, pPassword, pPasswordLength);
  }

  mFastRandomIndex = 0U;
  mWrapCount = 0U;
}

void FastRand::SetSeedBuffer(const unsigned char* pSeed, int pSeedLength) {
  FillRepeatedBuffer(mBufferSeed, pSeed, pSeedLength);
  mHasCustomSeedBuffer = (pSeed != nullptr && pSeedLength > 0);
}

void FastRand::SetPasswordExpander(bread::expansion::key_expansion::PasswordExpander* pPasswordExpander) {
  mPasswordExpander = pPasswordExpander;
}

void FastRand::SetWrapMode(FastRandWrapMode pWrapMode) {
  mWrapMode = pWrapMode;
}

unsigned char FastRand::Get() {
  if (mFastRandomIndex >= PASSWORD_EXPANDED_SIZE) {
    Wrap();
  }

  const unsigned char aByte = mFastRandomBuffer[mFastRandomIndex];
  ++mFastRandomIndex;
  return aByte;
}

unsigned int FastRand::GetInt() {
  unsigned int aValue = 0U;
  for (int aIndex = 0; aIndex < 4; ++aIndex) {
    aValue = (aValue << 8U) | static_cast<unsigned int>(Get());
  }
  return aValue;
}

std::uint64_t FastRand::WrapCount() const {
  return mWrapCount;
}

void FastRand::Wrap() {
  ++mWrapCount;

  for (int aIndex = 0; aIndex < PASSWORD_EXPANDED_SIZE; ++aIndex) {
    if (mWrapMode == FastRandWrapMode::kWrapAdd) {
      mBufferTemp[aIndex] = static_cast<unsigned char>(mFastRandomBuffer[aIndex] + mBufferSeed[aIndex]);
    } else {
      mBufferTemp[aIndex] = static_cast<unsigned char>(mFastRandomBuffer[aIndex] ^ mBufferSeed[aIndex]);
    }
  }

  if (mPasswordExpander != nullptr) {
    mPasswordExpander->Expand(mBufferTemp, PASSWORD_EXPANDED_SIZE, mFastRandomBuffer);
  } else {
    std::memcpy(mFastRandomBuffer, mBufferTemp, sizeof(mFastRandomBuffer));
  }

  mFastRandomIndex = 0U;
}

}  // namespace bread::fast_rand
