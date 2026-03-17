#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_

#include <cstddef>

#include "../../PeanutButter.hpp"
#include "../rng/Scrambler.hpp"

namespace peanutbutter::expansion::key_expansion {

class ByteTwister final : public peanutbutter::rng::Scrambler {
 public:
  enum Type {
    kType00 = 0,
    kType01,
    kType02,
    kType03,
    kType04,
    kType05,
    kType06,
    kType07,
    kType08,
    kType09,
    kType10,
    kType11,
    kType12,
    kType13,
    kType14,
    kType15,
    kTypeCount
  };

  static constexpr int kBufferLength = PASSWORD_EXPANDED_SIZE;

  explicit ByteTwister(Type pType = kType00)
      : mType(pType) {}

  void SetType(Type pType) { mType = pType; }
  Type CurrentType() const { return mType; }

  void Get(unsigned char* pSource,
           unsigned char* pWorker,
           unsigned char* pDestination,
           unsigned int pLength) override;

  static void TwistBytes(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned int pLength);
  static void TwistBytesByIndex(unsigned char pType,
                                unsigned char* pSource,
                                unsigned char* pWorker,
                                unsigned char* pDestination,
                                unsigned int pLength);

 private:
  Type mType;
};

}  // namespace peanutbutter::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
