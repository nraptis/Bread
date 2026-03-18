#include "Shuffler.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace peanutbutter::rng {

namespace {

[[noreturn]] void AbortInvalidShufflerUsage() {
  std::abort();
}

}  // namespace

Shuffler::Shuffler()
    : mSeedBuffer{},
      mResultBufferStorage{},
      mResultBuffer(nullptr),
      mSeedReadIndex(0U),
      mResultBufferWriteIndex(0U),
      mResultBufferLength(0U),
      mSeedBytesRemaining(0U),
      mResultBufferWriteProgress(0U) {
  std::memset(mSeedBuffer, 0, sizeof(mSeedBuffer));
  std::memset(mResultBufferStorage, 0, sizeof(mResultBufferStorage));
}

void Shuffler::Get(unsigned char* pDestination, int pDestinationLength) {
  if (pDestination == nullptr || pDestinationLength <= 0 || mResultBuffer == nullptr || mResultBufferLength == 0U) {
    AbortInvalidShufflerUsage();
  }

  if (static_cast<unsigned int>(pDestinationLength) != mResultBufferLength ||
      mResultBufferWriteProgress < mResultBufferLength) {
    AbortInvalidShufflerUsage();
  }

  if (pDestination == mResultBuffer) {
    return;
  }
  std::memcpy(pDestination, mResultBuffer, static_cast<std::size_t>(mResultBufferLength));
}

void Shuffler::InitializeSeedBuffer(unsigned char* pPassword, int pPasswordLength) {
  if (pPassword == nullptr || pPasswordLength <= 0 || pPasswordLength > kSeedBufferCapacity) {
    AbortInvalidShufflerUsage();
  }

  mResultBuffer = mResultBufferStorage;
  mResultBufferLength = static_cast<unsigned int>(pPasswordLength);
  mSeedReadIndex = 0U;
  mResultBufferWriteIndex = 0U;
  mSeedBytesRemaining = mResultBufferLength;
  mResultBufferWriteProgress = 0U;
  std::memcpy(mSeedBuffer, pPassword, static_cast<std::size_t>(pPasswordLength));
  std::memset(mResultBufferStorage, 0, sizeof(mResultBufferStorage));
  UseExternalResultBuffer(pPassword);
  if (pPasswordLength < kSeedBufferCapacity) {
    std::memset(mSeedBuffer + pPasswordLength, 0, static_cast<std::size_t>(kSeedBufferCapacity - pPasswordLength));
  }
}

void Shuffler::UseExternalResultBuffer(unsigned char* pBuffer) {
  if (pBuffer == nullptr) {
    mResultBuffer = mResultBufferStorage;
    if (mResultBufferLength > 0U) {
      std::memset(mResultBufferStorage, 0, static_cast<std::size_t>(mResultBufferLength));
    }
    return;
  }
  mResultBuffer = pBuffer;
}

bool Shuffler::SeedCanDequeue() const {
  return mResultBuffer != nullptr && mResultBufferLength > 0U && mSeedBytesRemaining > 0U;
}

unsigned char Shuffler::SeedDequeue() {
  if (!SeedCanDequeue()) {
    AbortInvalidShufflerUsage();
  }

  const unsigned char aByte = mSeedBuffer[mSeedReadIndex];
  mSeedReadIndex = (mSeedReadIndex + 1U) % mResultBufferLength;
  --mSeedBytesRemaining;
  return aByte;
}

void Shuffler::EnqueueByte(unsigned char pByte) {
  if (mResultBuffer == nullptr || mResultBufferLength == 0U) {
    AbortInvalidShufflerUsage();
  }

  mResultBuffer[mResultBufferWriteIndex] = pByte;
  mResultBufferWriteIndex = (mResultBufferWriteIndex + 1U) % mResultBufferLength;
  mResultBufferWriteProgress = std::min<std::uint64_t>(mResultBufferWriteProgress + 1U, mResultBufferLength);
}

}  // namespace peanutbutter::rng
