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
    peanutbutter::expansion::key_expansion::PasswordExpander::ExpandPassword(
        mType, pPassword, mWorker.data(), mBlock.data(), static_cast<unsigned int>(pPasswordLength));
    mCursor = 0;
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
    peanutbutter::expansion::key_expansion::PasswordExpander::ExpandPassword(
        mType, mBlock.data(), mWorker.data(), aNext.data(), PASSWORD_EXPANDED_SIZE);
    mBlock = aNext;
    mCursor = 0;
  }

  peanutbutter::expansion::key_expansion::PasswordExpander::Type mType;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> mBlock = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> mWorker = {};
  int mCursor = PASSWORD_EXPANDED_SIZE;
};

#endif  // BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_
