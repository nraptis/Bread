#ifndef BREAD_SRC_COUNTERS_MERSENNE_COUNTER_HPP_
#define BREAD_SRC_COUNTERS_MERSENNE_COUNTER_HPP_

#include <cstdint>

#include "src/rng/Counter.hpp"

class MersenneCounter final : public bread::rng::Counter {
 public:
  MersenneCounter();

  void Seed(unsigned char* pPassword, int pPasswordLength) override;
  void Get(unsigned char* pDestination, int pDestinationLength) override;
  unsigned char Get() override;

 private:
  static constexpr std::uint32_t kStateLength = 624U;
  static constexpr std::uint32_t kMiddleWord = 397U;
  static constexpr std::uint32_t kMatrixA = 0x9908B0DFU;
  static constexpr std::uint32_t kUpperMask = 0x80000000U;
  static constexpr std::uint32_t kLowerMask = 0x7FFFFFFFU;

  void Twist();
  std::uint32_t NextWord();

  std::uint32_t mState[kStateLength];
  std::uint32_t mIndex;
  std::uint32_t mWordBuffer;
  std::uint32_t mWordBufferUsed;
  bool mSeeded;
};

#endif  // BREAD_SRC_COUNTERS_MERSENNE_COUNTER_HPP_
