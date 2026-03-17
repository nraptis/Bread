#include "MersenneCounter.hpp"

MersenneCounter::MersenneCounter()
    : mState{},
      mIndex(kStateLength),
      mWordBuffer(0U),
      mWordBufferUsed(4U),
      mSeeded(false) {}

void MersenneCounter::Seed(unsigned char* pPassword, int pPasswordLength) {
  unsigned char aFallback = 0U;
  if (pPassword == nullptr || pPasswordLength <= 0) {
    pPassword = &aFallback;
    pPasswordLength = 1;
  }

  const std::uint32_t aPasswordLength = static_cast<std::uint32_t>(pPasswordLength);
  const std::uint32_t aStateByteLength = kStateLength * 4U;

  if (aPasswordLength == 1U) {
    const std::uint32_t aWord = static_cast<std::uint32_t>(pPassword[0]) * 0x01010101U;
    for (std::uint32_t aIndex = 0U; aIndex < kStateLength; ++aIndex) {
      mState[aIndex] = aWord;
    }
  } else {
    std::uint32_t aCursor = 0U;
    for (std::uint32_t aIndex = 0U; aIndex < kStateLength; ++aIndex) {
      std::uint32_t aWord = 0U;
      aWord |= static_cast<std::uint32_t>(pPassword[aCursor]) << 0U;
      ++aCursor;
      if (aCursor == aPasswordLength) {
        aCursor = 0U;
      }
      aWord |= static_cast<std::uint32_t>(pPassword[aCursor]) << 8U;
      ++aCursor;
      if (aCursor == aPasswordLength) {
        aCursor = 0U;
      }
      aWord |= static_cast<std::uint32_t>(pPassword[aCursor]) << 16U;
      ++aCursor;
      if (aCursor == aPasswordLength) {
        aCursor = 0U;
      }
      aWord |= static_cast<std::uint32_t>(pPassword[aCursor]) << 24U;
      ++aCursor;
      if (aCursor == aPasswordLength) {
        aCursor = 0U;
      }
      mState[aIndex] = aWord;
    }
  }

  auto aTwistInline = [this]() {
    for (std::uint32_t aIndex = 0; aIndex < kStateLength; ++aIndex) {
      const std::uint32_t aX =
          (mState[aIndex] & kUpperMask) | (mState[(aIndex + 1U) % kStateLength] & kLowerMask);
      std::uint32_t aXA = aX >> 1U;
      if ((aX & 1U) != 0U) {
        aXA ^= kMatrixA;
      }
      mState[aIndex] = mState[(aIndex + kMiddleWord) % kStateLength] ^ aXA;
    }
    mIndex = 0U;
  };

  aTwistInline();

  if (aPasswordLength > aStateByteLength) {
    std::uint32_t aRemainingOffset = aStateByteLength;
    std::uint32_t aWordIndex = 0U;
    while (aRemainingOffset < aPasswordLength) {
      std::uint32_t aWord = 0U;
      const std::uint32_t aRemain = aPasswordLength - aRemainingOffset;
      if (aRemain >= 4U) {
        aWord = static_cast<std::uint32_t>(pPassword[aRemainingOffset]) |
                (static_cast<std::uint32_t>(pPassword[aRemainingOffset + 1U]) << 8U) |
                (static_cast<std::uint32_t>(pPassword[aRemainingOffset + 2U]) << 16U) |
                (static_cast<std::uint32_t>(pPassword[aRemainingOffset + 3U]) << 24U);
      } else {
        for (std::uint32_t aByte = 0U; aByte < aRemain; ++aByte) {
          aWord |= static_cast<std::uint32_t>(pPassword[aRemainingOffset + aByte]) << (8U * aByte);
        }
      }

      mState[aWordIndex] ^= aWord;
      ++aWordIndex;
      aRemainingOffset += 4U;

      if (aWordIndex == kStateLength) {
        aWordIndex = 0U;
        aTwistInline();
      }
    }

    if (aWordIndex != 0U) {
      aTwistInline();
    }
  }

  mWordBuffer = 0U;
  mWordBufferUsed = 4U;
  mSeeded = true;
}

void MersenneCounter::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0) {
    return;
  }
  for (int aIndex = 0; aIndex < pDestinationLength; ++aIndex) {
    pDestination[aIndex] = Get();
  }
}

unsigned char MersenneCounter::Get() {
  if (!mSeeded) {
    Seed(nullptr, 0);
  }

  if (mWordBufferUsed >= 4U) {
    mWordBuffer = NextWord();
    mWordBufferUsed = 0U;
  }

  const unsigned char aByte =
      static_cast<unsigned char>((mWordBuffer >> (8U * mWordBufferUsed)) & 0xFFU);
  ++mWordBufferUsed;
  return aByte;
}

void MersenneCounter::Twist() {
  for (std::uint32_t aIndex = 0; aIndex < kStateLength; ++aIndex) {
    const std::uint32_t aX =
        (mState[aIndex] & kUpperMask) | (mState[(aIndex + 1U) % kStateLength] & kLowerMask);
    std::uint32_t aXA = aX >> 1U;
    if ((aX & 1U) != 0U) {
      aXA ^= kMatrixA;
    }
    mState[aIndex] = mState[(aIndex + kMiddleWord) % kStateLength] ^ aXA;
  }
  mIndex = 0U;
}

std::uint32_t MersenneCounter::NextWord() {
  if (mIndex >= kStateLength) {
    Twist();
  }

  std::uint32_t aY = mState[mIndex++];
  aY ^= (aY >> 11U);
  aY ^= (aY << 7U) & 0x9D2C5680U;
  aY ^= (aY << 15U) & 0xEFC60000U;
  aY ^= (aY >> 18U);
  return aY;
}
