#include "FastRand.hpp"

#include <cstring>

#include "../password_expanders/PasswordExpander.hpp"

namespace peanutbutter::fast_rand {

namespace {

void FillDoubledBuffer(unsigned char* pDestination, unsigned char* pSource, int pSourceLength) {
  peanutbutter::expansion::key_expansion::PasswordExpander::FillDoubledSource(
      pSource, (pSourceLength > 0) ? static_cast<unsigned int>(pSourceLength) : 0U, pDestination);
}

}  // namespace

FastRand::FastRand()
    : mSampleBuffer{},
      mBufferTemp{},
      mFastRandomBuffer{},
      mFastRandomIndex(0U),
      mWrapCount(0U),
      mCurrentTwistType(0U),
      mSaltBuffer{},
      mKeyStack{},
      mNextRoundKeyBuffer{} {}

void FastRand::Seed(unsigned char* pPassword, int pPasswordLength) {
  BuildSampleBuffer(pPassword, pPasswordLength);
  mCurrentTwistType = SelectTwistTypeFromBuffer(mSampleBuffer);
  std::memset(mKeyStack, 0, sizeof(mKeyStack));
  std::memset(mNextRoundKeyBuffer, 0, sizeof(mNextRoundKeyBuffer));
  std::memset(mSaltBuffer, 0, sizeof(mSaltBuffer));
  peanutbutter::expansion::key_expansion::ByteTwister::SeedKeyByIndex(
      mCurrentTwistType, mSampleBuffer, mKeyStack, PASSWORD_EXPANDED_SIZE);
  peanutbutter::expansion::key_expansion::ByteTwister::SeedSaltByIndex(
      mCurrentTwistType, mSampleBuffer, mSaltBuffer, PASSWORD_EXPANDED_SIZE);
  peanutbutter::expansion::key_expansion::ByteTwister::TwistBlockByIndex(
      mCurrentTwistType,
      mSampleBuffer,
      mBufferTemp,
      mFastRandomBuffer,
      0U,
      mSaltBuffer,
      mKeyStack,
      PASSWORD_EXPANDED_SIZE);
  peanutbutter::expansion::key_expansion::ByteTwister::PushKeyRoundByIndex(
      mCurrentTwistType,
      mFastRandomBuffer,
      mSaltBuffer,
      mKeyStack,
      mNextRoundKeyBuffer,
      PASSWORD_EXPANDED_SIZE);
  mFastRandomIndex = 0U;
  mWrapCount = 0U;
}

void FastRand::Mix(unsigned char* pBuffer, int pBufferLength) {
  if (pBuffer == nullptr || pBufferLength <= 0) {
    return;
  }

  for (int aIndex = 0; aIndex < pBufferLength; ++aIndex) {
    pBuffer[aIndex] = Get();
  }
}

void FastRand::BuildSampleBuffer(unsigned char* pPassword, int pPasswordLength) {
  FillDoubledBuffer(mSampleBuffer, pPassword, pPasswordLength);
}

unsigned char FastRand::SelectTwistTypeFromBuffer(const unsigned char* pBuffer) const {
  if (pBuffer == nullptr) {
    return 0U;
  }
  const unsigned int aFirst = static_cast<unsigned int>(pBuffer[0]);
  const unsigned int aLast = static_cast<unsigned int>(pBuffer[PASSWORD_EXPANDED_SIZE - 1]);
  const unsigned int aMagicIndex = ((aFirst << 8U) | aLast) % static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE);
  const unsigned char aMagicByte = pBuffer[aMagicIndex];
  return static_cast<unsigned char>(aMagicByte & 15U);
}

unsigned char FastRand::Get() {
  if (mFastRandomIndex >= PASSWORD_EXPANDED_SIZE) {
    Wrap();
  }

  const unsigned char aByte = mFastRandomBuffer[mFastRandomIndex];
  ++mFastRandomIndex;
  return aByte;
}

unsigned char FastRand::Get(int pMax) {
  if (pMax <= 0) {
    return 0U;
  }
  return static_cast<unsigned char>(static_cast<int>(Get()) % pMax);
}

unsigned int FastRand::GetInt() {
  unsigned int aValue = 0U;
  for (int aIndex = 0; aIndex < 4; ++aIndex) {
    aValue = (aValue << 8U) | static_cast<unsigned int>(Get());
  }
  return aValue;
}

unsigned int FastRand::NextRand(int pMax) {
  if (pMax <= 0) {
    return 0U;
  }
  return GetInt() % static_cast<unsigned int>(pMax);
}

std::uint64_t FastRand::WrapCount() const {
  return mWrapCount;
}

unsigned char FastRand::CurrentTwistType() const {
  return mCurrentTwistType;
}

void FastRand::Wrap() {
  ++mWrapCount;

  mCurrentTwistType = SelectTwistTypeFromBuffer(mFastRandomBuffer);
  std::memcpy(mSampleBuffer, mFastRandomBuffer, sizeof(mSampleBuffer));
  peanutbutter::expansion::key_expansion::ByteTwister::SeedSaltByIndex(
      mCurrentTwistType, mSampleBuffer, mSaltBuffer, PASSWORD_EXPANDED_SIZE);
  peanutbutter::expansion::key_expansion::ByteTwister::TwistBlockByIndex(
      mCurrentTwistType,
      mSampleBuffer,
      mBufferTemp,
      mFastRandomBuffer,
      static_cast<unsigned int>(mWrapCount),
      mSaltBuffer,
      mKeyStack,
      PASSWORD_EXPANDED_SIZE);
  peanutbutter::expansion::key_expansion::ByteTwister::PushKeyRoundByIndex(
      mCurrentTwistType,
      mFastRandomBuffer,
      mSaltBuffer,
      mKeyStack,
      mNextRoundKeyBuffer,
      PASSWORD_EXPANDED_SIZE);
  mFastRandomIndex = 0U;
}

}  // namespace peanutbutter::fast_rand
