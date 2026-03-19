#include "PasswordExpander.hpp"

#include <array>
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

void PasswordExpander::ExpandPassword(
    Type pType,
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDestination,
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
    unsigned int pLength) {
  ExpandPasswordByIndex(
      static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pKeyBuffer, pNextRoundKeyBuffer, pLength);
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

void PasswordExpander::ExpandPasswordBlocks(
    Type pType,
    unsigned char* pSource,
    unsigned int pSourceLength,
    unsigned char* pWorker,
    unsigned char* pDestination,
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
    unsigned int pOutputLength) {
  ExpandPasswordBlocksByIndex(static_cast<unsigned char>(pType),
                              pSource,
                              pSourceLength,
                              pWorker,
                              pDestination,
                              pKeyBuffer,
                              pNextRoundKeyBuffer,
                              pOutputLength);
}

void PasswordExpander::ExpandPasswordByIndex(unsigned char pType,
                                             unsigned char* pSource,
                                             unsigned char* pWorker,
                                             unsigned char* pDestination,
                                             unsigned int pLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  ExpandPasswordByIndex(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);
}

void PasswordExpander::ExpandPasswordByIndex(
    unsigned char pType,
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDestination,
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
    unsigned int pLength) {
  if (pDestination == nullptr) {
    return;
  }
  if (pLength == 0U || pLength > static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE)) {
    std::abort();
  }

  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aSourceBuffer{};
  FillRepeatedSource(pSource, pLength, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE), aSourceBuffer.data());
  ByteTwister::TwistBytesByIndex(
      pType, aSourceBuffer.data(), pWorker, pDestination, pKeyBuffer, pNextRoundKeyBuffer,
      static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
}

void PasswordExpander::ExpandPasswordBlocksByIndex(unsigned char pType,
                                                   unsigned char* pSource,
                                                   unsigned int pSourceLength,
                                                   unsigned char* pWorker,
                                                   unsigned char* pDestination,
                                                   unsigned int pOutputLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  ExpandPasswordBlocksByIndex(
      pType, pSource, pSourceLength, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pOutputLength);
}

void PasswordExpander::ExpandPasswordBlocksByIndex(
    unsigned char pType,
    unsigned char* pSource,
    unsigned int pSourceLength,
    unsigned char* pWorker,
    unsigned char* pDestination,
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
    unsigned int pOutputLength) {
  if (pDestination == nullptr) {
    return;
  }
  constexpr unsigned int kBlockLength = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE);
  if (pOutputLength == 0U || (pOutputLength % kBlockLength) != 0U) {
    std::abort();
  }

  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aSourceBuffer{};
  FillRepeatedSource(pSource, pSourceLength, kBlockLength, aSourceBuffer.data());
  ByteTwister::TwistBytesByIndex(
      pType, aSourceBuffer.data(), pWorker, pDestination, pKeyBuffer, pNextRoundKeyBuffer, pOutputLength);
}

}  // namespace peanutbutter::expansion::key_expansion
