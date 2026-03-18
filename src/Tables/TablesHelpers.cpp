#include "src/Tables/TablesHelpers.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "src/Tables/Tables.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"

namespace peanutbutter::tables::helpers {

namespace {

void FastFill(unsigned char* pBuffer, int pLength, int pSeed) {
  if (pBuffer == nullptr || pLength <= 0) {
    return;
  }

  std::uint32_t aState = static_cast<std::uint32_t>(pSeed);
  aState ^= 0xA5A55A5AU;
  if (aState == 0U) {
    aState = 0x13579BDFU;
  }

  for (int aIndex = 0; aIndex < pLength; ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    aState += static_cast<std::uint32_t>((aIndex + 1) * 17);
    pBuffer[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

template <std::size_t kCount>
void ShuffleInstructions(std::array<FillInstruction, kCount>& pInstructions, std::size_t pScratchOffset) {
  for (std::size_t aIndex = 0; aIndex < kCount; ++aIndex) {
    const std::size_t aRemaining = kCount - aIndex;
    const std::size_t aSwapIndex =
        aIndex + (static_cast<std::size_t>(Tables::mScratch[(pScratchOffset + aIndex) % Tables::kScratchSize]) %
                  aRemaining);
    std::swap(pInstructions[aIndex], pInstructions[aSwapIndex]);
  }
}

FillInstruction NextExpanderInstruction(const std::array<unsigned char, PasswordExpander::kTypeCount>& pStack,
                                        std::size_t* pIndex) {
  if (pIndex == nullptr) {
    return {TableFillKind::kPasswordExpander, 0U};
  }
  const std::size_t aStackIndex = (*pIndex) % pStack.size();
  ++(*pIndex);
  return {TableFillKind::kPasswordExpander, pStack[aStackIndex]};
}

bool FillCounter(TableFillKind pKind,
                 unsigned char* pPassword,
                 int pPasswordLength,
                 unsigned char* pDestination,
                 std::size_t pDestinationLength) {
  if (pDestination == nullptr) {
    return false;
  }

  switch (pKind) {
    case TableFillKind::kAesCounter: {
      AESCounter aCounter;
      aCounter.Seed(pPassword, pPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case TableFillKind::kAriaCounter: {
      ARIA256Counter aCounter;
      aCounter.Seed(pPassword, pPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case TableFillKind::kChaChaCounter: {
      ChaCha20Counter aCounter;
      aCounter.Seed(pPassword, pPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case TableFillKind::kPasswordExpander:
      break;
  }
  return false;
}

bool FillExpander(unsigned char pExpanderIndex,
                  unsigned char* pPassword,
                  int pPasswordLength,
                  unsigned char* pDestination,
                  std::size_t pDestinationLength) {
  if (pDestination == nullptr) {
    return false;
  }

  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  const unsigned char aNormalizedIndex =
      static_cast<unsigned char>(pExpanderIndex % static_cast<unsigned char>(PasswordExpander::kTypeCount));
  PasswordExpander::ExpandPasswordBlocksByIndex(aNormalizedIndex,
                                                pPassword,
                                                (pPasswordLength > 0) ? static_cast<unsigned int>(pPasswordLength) : 0U,
                                                aWorker.data(),
                                                pDestination,
                                                static_cast<unsigned int>(pDestinationLength));
  return true;
}

}  // namespace

std::string BuildLogMessage(const std::string& pMessage) {
  return "[Expansion] " + pMessage;
}

void LogStatus(Logger* pLogger, const std::string& pMessage) {
  if (pLogger != nullptr) {
    pLogger->LogStatus(BuildLogMessage(pMessage));
  }
}

void LogError(Logger* pLogger, const std::string& pMessage) {
  if (pLogger != nullptr) {
    pLogger->LogError(BuildLogMessage(pMessage));
  }
}

double ClampFraction(double pFraction) {
  if (pFraction < 0.0) {
    return 0.0;
  }
  if (pFraction > 1.0) {
    return 1.0;
  }
  return pFraction;
}

std::string ResolveModeName(const char* pModeName) {
  if (pModeName == nullptr || *pModeName == '\0') {
    return "Bundle";
  }
  return std::string(pModeName);
}

std::size_t GameStride(GameStyle pStyle) {
  return (pStyle == GameStyle::kSparse) ? 4U : 1U;
}

std::size_t MazeStride(MazeStyle pStyle) {
  return (pStyle == MazeStyle::kSparse) ? 4U : 1U;
}

bool ShouldCancel(const LaunchRequest& pRequest) {
  return pRequest.mShouldCancel != nullptr && pRequest.mShouldCancel(pRequest.mCancelUserData);
}

void BuildScratch(const LaunchRequest& pRequest) {
  if (pRequest.mPassword == nullptr || pRequest.mPasswordLength <= 0) {
    std::memset(Tables::mScratch, 0, sizeof(Tables::mScratch));
    return;
  }

  for (std::size_t aIndex = 0; aIndex < Tables::kScratchSize; ++aIndex) {
    Tables::mScratch[aIndex] =
        pRequest.mPassword[static_cast<std::size_t>(aIndex) % static_cast<std::size_t>(pRequest.mPasswordLength)];
  }
}

std::array<unsigned char, PasswordExpander::kTypeCount> BuildExpanderStack() {
  std::array<unsigned char, PasswordExpander::kTypeCount> aStack = {};
  for (int aIndex = 0; aIndex < PasswordExpander::kTypeCount; ++aIndex) {
    aStack[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>(aIndex);
  }

  for (std::size_t aIndex = 0; aIndex < aStack.size(); ++aIndex) {
    const std::size_t aRemaining = aStack.size() - aIndex;
    const std::size_t aSwapIndex =
        aIndex + (static_cast<std::size_t>(Tables::mScratch[(3U + aIndex) % Tables::kScratchSize]) % aRemaining);
    std::swap(aStack[aIndex], aStack[aSwapIndex]);
  }
  return aStack;
}

std::array<FillInstruction, Tables::kL1TableCount> BuildL1Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex) {
  const bool aUseAriaOnL1 = (Tables::mScratch[0] & 1U) == 0U;
  std::array<FillInstruction, Tables::kL1TableCount> aPlan = {{
      {TableFillKind::kAesCounter, 0U},
      {aUseAriaOnL1 ? TableFillKind::kAriaCounter : TableFillKind::kChaChaCounter, 0U},
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
  }};
  ShuffleInstructions(aPlan, 1U);
  return aPlan;
}

std::array<FillInstruction, Tables::kL2TableCount> BuildL2Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex) {
  const bool aUseAriaOnL1 = (Tables::mScratch[0] & 1U) == 0U;
  std::array<FillInstruction, Tables::kL2TableCount> aPlan = {{
      {TableFillKind::kAesCounter, 0U},
      {aUseAriaOnL1 ? TableFillKind::kChaChaCounter : TableFillKind::kAriaCounter, 0U},
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
  }};
  ShuffleInstructions(aPlan, 7U);
  return aPlan;
}

std::array<FillInstruction, Tables::kL3TableCount> BuildL3Plan(
    const std::array<unsigned char, PasswordExpander::kTypeCount>& pExpanderStack,
    std::size_t* pExpanderIndex) {
  std::array<FillInstruction, Tables::kL3TableCount> aPlan = {{
      {TableFillKind::kAesCounter, 0U},
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
      NextExpanderInstruction(pExpanderStack, pExpanderIndex),
  }};
  ShuffleInstructions(aPlan, 13U);
  return aPlan;
}

std::array<FillInstruction, Tables::kTableCount> BuildTablePlan() {
  const auto aExpanderStack = BuildExpanderStack();
  std::size_t aExpanderIndex = 0U;
  const auto aL1Plan = BuildL1Plan(aExpanderStack, &aExpanderIndex);
  const auto aL2Plan = BuildL2Plan(aExpanderStack, &aExpanderIndex);
  const auto aL3Plan = BuildL3Plan(aExpanderStack, &aExpanderIndex);

  std::array<FillInstruction, Tables::kTableCount> aPlan = {};
  for (std::size_t aIndex = 0; aIndex < Tables::kL1TableCount; ++aIndex) {
    aPlan[aIndex] = aL1Plan[aIndex];
  }
  for (std::size_t aIndex = 0; aIndex < Tables::kL2TableCount; ++aIndex) {
    aPlan[Tables::kL1TableCount + aIndex] = aL2Plan[aIndex];
  }
  for (std::size_t aIndex = 0; aIndex < Tables::kL3TableCount; ++aIndex) {
    aPlan[Tables::kL1TableCount + Tables::kL2TableCount + aIndex] = aL3Plan[aIndex];
  }
  return aPlan;
}

bool FillTable(const FillInstruction& pInstruction,
               unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pDestination,
               std::size_t pDestinationLength) {
  if (pInstruction.mKind == TableFillKind::kPasswordExpander) {
    return FillExpander(pInstruction.mExpanderIndex, pPassword, pPasswordLength, pDestination, pDestinationLength);
  }
  return FillCounter(pInstruction.mKind, pPassword, pPasswordLength, pDestination, pDestinationLength);
}

bool FastFillTable(std::size_t pTableIndex,
                   const FillInstruction& pInstruction,
                   unsigned char* pDestination,
                   std::size_t pDestinationLength) {
  int aSeed = static_cast<int>((pTableIndex + 1U) * 97U);
  aSeed += static_cast<int>(Tables::mScratch[pTableIndex % Tables::kScratchSize]) * 257;
  aSeed += static_cast<int>(pDestinationLength & 0x7FFFU);
  if (pInstruction.mKind == TableFillKind::kPasswordExpander) {
    aSeed += static_cast<int>((pInstruction.mExpanderIndex + 1U) * 131U);
  } else {
    aSeed += static_cast<int>((static_cast<unsigned int>(pInstruction.mKind) + 1U) * 1009U);
  }
  FastFill(pDestination, static_cast<int>(pDestinationLength), aSeed);
  return true;
}

std::string FillLabel(const FillInstruction& pInstruction) {
  switch (pInstruction.mKind) {
    case TableFillKind::kAesCounter:
      return "aes";
    case TableFillKind::kAriaCounter:
      return "aria";
    case TableFillKind::kChaChaCounter:
      return "chacha";
    case TableFillKind::kPasswordExpander:
      return "p_" + std::to_string(pInstruction.mExpanderIndex);
  }
  return "unknown";
}

std::size_t CountProcessedBlocks(std::size_t pBlockCount, std::size_t pStride) {
  if (pBlockCount == 0U || pStride == 0U) {
    return 0U;
  }
  return 1U + ((pBlockCount - 1U) / pStride);
}

std::size_t CountGameBlocks(std::size_t pTableSize, GameStyle pStyle) {
  if (pStyle == GameStyle::kNone || (pTableSize % static_cast<std::size_t>(games::GameBoard::kSeedBufferCapacity)) != 0U) {
    return 0U;
  }
  const std::size_t aBlockCount = pTableSize / static_cast<std::size_t>(games::GameBoard::kSeedBufferCapacity);
  return CountProcessedBlocks(aBlockCount, GameStride(pStyle));
}

std::size_t CountMazeBlocks(std::size_t pTableSize, MazeStyle pStyle) {
  if (pStyle == MazeStyle::kNone || (pTableSize % static_cast<std::size_t>(maze::MazeDirector::kSeedBufferCapacity)) != 0U) {
    return 0U;
  }
  const std::size_t aBlockCount = pTableSize / static_cast<std::size_t>(maze::MazeDirector::kSeedBufferCapacity);
  return CountProcessedBlocks(aBlockCount, MazeStride(pStyle));
}

void ShufflePlayOrderFromBuffer(const unsigned char* pBuffer, std::size_t pLength, int* pPlayOrder, int pCount) {
  if (pBuffer == nullptr || pLength == 0U || pPlayOrder == nullptr || pCount <= 0) {
    return;
  }

  for (int aIndex = 0; aIndex < pCount; ++aIndex) {
    pPlayOrder[aIndex] = aIndex;
  }

  for (int aIndex = 0; aIndex < pCount; ++aIndex) {
    const int aWhich = static_cast<int>(pBuffer[static_cast<std::size_t>(aIndex) % pLength] %
                                        static_cast<unsigned char>(pCount));
    const int aTemp = pPlayOrder[aIndex];
    pPlayOrder[aIndex] = pPlayOrder[aWhich];
    pPlayOrder[aWhich] = aTemp;
  }
}

}  // namespace peanutbutter::tables::helpers
