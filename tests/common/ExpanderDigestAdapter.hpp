#ifndef BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_
#define BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_

#include <algorithm>
#include <array>
#include <cstring>

#include "src/core/Bread.hpp"
#include "src/expansion/key_expansion/PasswordExpander.hpp"
#include "src/rng/Digest.hpp"

class ExpanderDigestAdapter final : public bread::rng::Digest {
 public:
  explicit ExpanderDigestAdapter(bread::expansion::key_expansion::PasswordExpander* pExpander)
      : mExpander(pExpander) {
    std::memset(mBlock.data(), 0, mBlock.size());
  }

  void Seed(unsigned char* pPassword, int pPasswordLength) override {
    if (mExpander == nullptr) {
      return;
    }
    mExpander->Expand(pPassword, pPasswordLength, mBlock.data());
    mCursor = 0;
  }

  void Get(unsigned char* pDestination, int pDestinationLength) override {
    if (mExpander == nullptr || pDestination == nullptr || pDestinationLength <= 0) {
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
    if (mExpander == nullptr) {
      return;
    }
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aNext = {};
    mExpander->Expand(mBlock.data(), PASSWORD_EXPANDED_SIZE, aNext.data());
    mBlock = aNext;
    mCursor = 0;
  }

  bread::expansion::key_expansion::PasswordExpander* mExpander = nullptr;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> mBlock = {};
  int mCursor = PASSWORD_EXPANDED_SIZE;
};

#endif  // BREAD_TESTS_COMMON_EXPANDER_DIGEST_ADAPTER_HPP_
