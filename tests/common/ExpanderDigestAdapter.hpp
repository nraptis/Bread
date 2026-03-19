#ifndef BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_
#define BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_

#include <algorithm>
#include <array>
#include <cstring>

#include "src/PeanutButter.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "src/Tables/rng/Digest.hpp"

class ExpanderDigestAdapter final : public peanutbutter::rng::Digest {
 public:
  explicit ExpanderDigestAdapter(
      peanutbutter::expansion::key_expansion::PasswordExpander::Type pType)
      : mType(pType) {
    std::memset(mBlock.data(), 0, mBlock.size());
  }

  void Seed(unsigned char* pPassword, int pPasswordLength) override {
    peanutbutter::expansion::key_expansion::PasswordExpander::FillDoubledSource(
        pPassword, static_cast<unsigned int>((pPasswordLength > 0) ? pPasswordLength : 0), mBlock.data());
    std::memset(mKeyBuffer, 0, sizeof(mKeyBuffer));
    std::memset(mSaltBuffer, 0, sizeof(mSaltBuffer));
    std::memset(mNextRoundKeyBuffer, 0, sizeof(mNextRoundKeyBuffer));
    peanutbutter::expansion::key_expansion::ByteTwister::SeedKey(
        mType, mBlock.data(), mKeyBuffer, PASSWORD_EXPANDED_SIZE);
    peanutbutter::expansion::key_expansion::ByteTwister::SeedSalt(
        mType, mBlock.data(), mSaltBuffer, PASSWORD_EXPANDED_SIZE);
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aNext = {};
    peanutbutter::expansion::key_expansion::ByteTwister::TwistBlock(
        mType, mBlock.data(), mWorker.data(), aNext.data(), 0U, mSaltBuffer, mKeyBuffer, PASSWORD_EXPANDED_SIZE);
    peanutbutter::expansion::key_expansion::ByteTwister::PushKeyRound(
        mType, aNext.data(), mSaltBuffer, mKeyBuffer, mNextRoundKeyBuffer, PASSWORD_EXPANDED_SIZE);
    mBlock = aNext;
    mCursor = 0;
    mRound = 1U;
  }

  void Get(unsigned char* pDestination, int pDestinationLength) override {
    if (pDestination == nullptr || pDestinationLength <= 0) {
      return;
    }

    int aRemaining = pDestinationLength;
    int aOffset = 0;
    while (aRemaining > 0) {
      if (mCursor >= PASSWORD_EXPANDED_SIZE) {
        Refill();
      }

      const int aAvailable = PASSWORD_EXPANDED_SIZE - mCursor;
      const int aTake = std::min(aRemaining, aAvailable);
      std::memcpy(pDestination + aOffset, mBlock.data() + mCursor, static_cast<std::size_t>(aTake));
      mCursor += aTake;
      aOffset += aTake;
      aRemaining -= aTake;
    }
  }

 private:
  void Refill() {
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aNext = {};
    peanutbutter::expansion::key_expansion::ByteTwister::TwistBlock(
        mType, mBlock.data(), mWorker.data(), aNext.data(), mRound, mSaltBuffer, mKeyBuffer, PASSWORD_EXPANDED_SIZE);
    peanutbutter::expansion::key_expansion::ByteTwister::PushKeyRound(
        mType, aNext.data(), mSaltBuffer, mKeyBuffer, mNextRoundKeyBuffer, PASSWORD_EXPANDED_SIZE);
    mBlock = aNext;
    mCursor = 0;
    ++mRound;
  }

  peanutbutter::expansion::key_expansion::PasswordExpander::Type mType;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> mBlock = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> mWorker = {};
  unsigned char mSaltBuffer[twist::kSaltBytes] = {};
  unsigned char mNextRoundKeyBuffer[twist::kRoundKeyBytes] = {};
  unsigned char mKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes] = {};
  int mCursor = PASSWORD_EXPANDED_SIZE;
  unsigned int mRound = 0U;
};

#endif  // BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_
