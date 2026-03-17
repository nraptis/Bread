#include "PasswordExpander.hpp"

#include <cstdlib>
#include <cstring>

namespace peanutbutter::expansion::key_expansion {

namespace {

void FillRepeatedSource(const unsigned char* pPassword,
                        unsigned int pPasswordLength,
                        unsigned int pDestinationLength,
                        unsigned char* pSourceBuffer) {
  if (pSourceBuffer == nullptr) {
    return;
  }
  if (pPassword == nullptr || pPasswordLength == 0U) {
    std::memset(pSourceBuffer, 0, pDestinationLength);
    return;
  }

  const unsigned int aInitialCopy = (pPasswordLength < pDestinationLength) ? pPasswordLength : pDestinationLength;
  std::memcpy(pSourceBuffer, pPassword, static_cast<std::size_t>(aInitialCopy));
  unsigned int aFilled = aInitialCopy;
  while (aFilled < pDestinationLength) {
    const unsigned int aRemaining = pDestinationLength - aFilled;
    const unsigned int aChunk = (aRemaining < aFilled) ? aRemaining : aFilled;
    std::memcpy(pSourceBuffer + aFilled, pSourceBuffer, static_cast<std::size_t>(aChunk));
    aFilled += aChunk;
  }
}

}  // namespace

void PasswordExpander::FillDoubledSource(const unsigned char* pPassword,
                                         unsigned int pPasswordLength,
                                         unsigned char* pSourceBuffer) {
  FillRepeatedSource(pPassword, pPasswordLength, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE), pSourceBuffer);
}

void PasswordExpander::Get(unsigned char* pSource,
                           unsigned char* pWorker,
                           unsigned char* pDestination,
                           unsigned int pLength) {
  ExpandPassword(mType, pSource, pWorker, pDestination, pLength);
}

void PasswordExpander::ExpandPassword(Type pType,
                                      unsigned char* pSource,
                                      unsigned char* pWorker,
                                      unsigned char* pDestination,
                                      unsigned int pLength) {
  ExpandPasswordByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pLength);
}

void PasswordExpander::ExpandPasswordBlocks(Type pType,
                                            unsigned char* pSource,
                                            unsigned int pSourceLength,
                                            unsigned char* pWorker,
                                            unsigned char* pDestination,
                                            unsigned int pOutputLength) {
  ExpandPasswordBlocksByIndex(static_cast<unsigned char>(pType),
                              pSource,
                              pSourceLength,
                              pWorker,
                              pDestination,
                              pOutputLength);
}

void PasswordExpander::ExpandPasswordByIndex(unsigned char pType,
                                             unsigned char* pSource,
                                             unsigned char* pWorker,
                                             unsigned char* pDestination,
                                             unsigned int pLength) {
  if (pDestination == nullptr) {
    return;
  }
  if (pLength == 0U || pLength > static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE)) {
    std::abort();
  }

  FillRepeatedSource(pSource, pLength, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE), pDestination);
  ByteTwister::TwistBytesByIndex(pType, pDestination, pWorker, pDestination,
                                 static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
}

void PasswordExpander::ExpandPasswordBlocksByIndex(unsigned char pType,
                                                   unsigned char* pSource,
                                                   unsigned int pSourceLength,
                                                   unsigned char* pWorker,
                                                   unsigned char* pDestination,
                                                   unsigned int pOutputLength) {
  if (pDestination == nullptr) {
    return;
  }
  constexpr unsigned int kBlockLength = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE);
  if (pOutputLength == 0U || (pOutputLength % kBlockLength) != 0U) {
    std::abort();
  }

  FillRepeatedSource(pSource, pSourceLength, kBlockLength, pDestination);
  ByteTwister::TwistBytesByIndex(pType, pDestination, pWorker, pDestination, kBlockLength);

  for (unsigned int aOffset = kBlockLength; aOffset < pOutputLength; aOffset += kBlockLength) {
    std::memcpy(pDestination + aOffset, pDestination + (aOffset - kBlockLength), static_cast<std::size_t>(kBlockLength));
    ByteTwister::TwistBytesByIndex(pType, pDestination + aOffset, pWorker, pDestination + aOffset, kBlockLength);
  }
}

}  // namespace peanutbutter::expansion::key_expansion
