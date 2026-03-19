#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../../PeanutButter.hpp"
#include "../rng/Scrambler.hpp"

namespace twist {

inline constexpr int kPasswordExpandedSize = static_cast<int>(peanutbutter::kPasswordExpandedSize);
inline constexpr std::size_t kRoundKeyStackDepth = 16u;
inline constexpr std::size_t kRoundKeyBytes = 32u;
inline constexpr std::size_t kSaltBytes = 32u;

inline int WrapRange(int pIndex, int pStart, int pEnd) {
  const int aSpan = pEnd - pStart;
  if (aSpan <= 0) {
    return pStart;
  }
  int aWrapped = (pIndex - pStart) % aSpan;
  if (aWrapped < 0) {
    aWrapped += aSpan;
  }
  return pStart + aWrapped;
}

inline std::uint32_t RotateLeft32(std::uint32_t pValue, unsigned int pBits) {
  const unsigned int aShift = pBits & 31U;
  if (aShift == 0U) {
    return pValue;
  }
  return (pValue << aShift) | (pValue >> (32U - aShift));
}

inline unsigned char KeyStackByte(
    const unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    std::size_t pPlaneIndex,
    std::size_t pByteIndex) {
  return pKeyStack[pPlaneIndex % kRoundKeyStackDepth][pByteIndex % kRoundKeyBytes];
}

inline void RotateKeyStack(
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    const unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes]) {
  for (std::size_t aPlane = kRoundKeyStackDepth - 1U; aPlane > 0U; --aPlane) {
    std::memcpy(pKeyStack[aPlane], pKeyStack[aPlane - 1U], kRoundKeyBytes);
  }
  std::memcpy(pKeyStack[0], pNextRoundKeyBuffer, kRoundKeyBytes);
}

}  // namespace twist

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

  static constexpr int kBufferLength = static_cast<int>(peanutbutter::kPasswordExpandedSize);

  explicit ByteTwister(Type pType = kType00)
      : mType(pType) {}

  void SetType(Type pType) { mType = pType; }
  Type CurrentType() const { return mType; }

  void Get(unsigned char* pSource,
           unsigned char* pWorker,
           unsigned char* pDestination,
           unsigned int pLength) override;

  static void SeedKey(Type pType,
                      unsigned char* pSource,
                      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                      unsigned int pLength);
  static void SeedKeyByIndex(
      unsigned char pType,
      unsigned char* pSource,
      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
      unsigned int pLength);

  static void SeedSalt(Type pType,
                       unsigned char* pSource,
                       unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                       unsigned int pLength);
  static void SeedSaltByIndex(unsigned char pType,
                              unsigned char* pSource,
                              unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                              unsigned int pLength);

  static void TwistBlock(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned int pRound,
                         const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                         unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                         unsigned int pLength);
  static void TwistBlockByIndex(
      unsigned char pType,
      unsigned char* pSource,
      unsigned char* pWorker,
      unsigned char* pDestination,
      unsigned int pRound,
      const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
      unsigned int pLength);

  static void PushKeyRound(Type pType,
                           unsigned char* pDestination,
                           const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                           unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                           unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                           unsigned int pLength);
  static void PushKeyRoundByIndex(
      unsigned char pType,
      unsigned char* pDestination,
      const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
      unsigned int pLength);

  static void TwistBytes(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned int pLength);
  static void TwistBytes(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                         unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                         unsigned int pLength);
  static void TwistBytesByIndex(unsigned char pType,
                                unsigned char* pSource,
                                unsigned char* pWorker,
                                unsigned char* pDestination,
                                unsigned int pLength);
  static void TwistBytesByIndex(
      unsigned char pType,
      unsigned char* pSource,
      unsigned char* pWorker,
      unsigned char* pDestination,
      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
      unsigned int pLength);

 private:
  Type mType;
};

}  // namespace peanutbutter::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
