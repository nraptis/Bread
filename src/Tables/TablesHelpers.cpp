#include "src/Tables/TablesHelpers.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace peanutbutter::tables::helpers {

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr TableFillKind kBaseFillKinds[Tables::kBaseFillKindCount] = {
    TableFillKind::kAesCounter1,
    TableFillKind::kAesCounter2,
    TableFillKind::kAesCounter3,
    TableFillKind::kAriaCounter,
    TableFillKind::kChaChaCounter,
    TableFillKind::kPasswordExpander00,
    TableFillKind::kPasswordExpander01,
    TableFillKind::kPasswordExpander02,
    TableFillKind::kPasswordExpander03,
    TableFillKind::kPasswordExpander04,
    TableFillKind::kPasswordExpander05,
    TableFillKind::kPasswordExpander06,
    TableFillKind::kPasswordExpander07,
    TableFillKind::kPasswordExpander08,
    TableFillKind::kPasswordExpander09,
    TableFillKind::kPasswordExpander10,
    TableFillKind::kPasswordExpander11,
    TableFillKind::kPasswordExpander12,
    TableFillKind::kPasswordExpander13,
    TableFillKind::kPasswordExpander14,
    TableFillKind::kPasswordExpander15,
};

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

unsigned char ExpanderIndexForKind(TableFillKind pKind) {
  switch (pKind) {
    case TableFillKind::kPasswordExpander00:
      return 0U;
    case TableFillKind::kPasswordExpander01:
      return 1U;
    case TableFillKind::kPasswordExpander02:
      return 2U;
    case TableFillKind::kPasswordExpander03:
      return 3U;
    case TableFillKind::kPasswordExpander04:
      return 4U;
    case TableFillKind::kPasswordExpander05:
      return 5U;
    case TableFillKind::kPasswordExpander06:
      return 6U;
    case TableFillKind::kPasswordExpander07:
      return 7U;
    case TableFillKind::kPasswordExpander08:
      return 8U;
    case TableFillKind::kPasswordExpander09:
      return 9U;
    case TableFillKind::kPasswordExpander10:
      return 10U;
    case TableFillKind::kPasswordExpander11:
      return 11U;
    case TableFillKind::kPasswordExpander12:
      return 12U;
    case TableFillKind::kPasswordExpander13:
      return 13U;
    case TableFillKind::kPasswordExpander14:
      return 14U;
    case TableFillKind::kPasswordExpander15:
      return 15U;
    default:
      return 0U;
  }
}

void ReverseBytes(unsigned char* pBuffer, std::size_t pLength) {
  if (pBuffer == nullptr || pLength <= 1U) {
    return;
  }
  std::size_t aLeft = 0U;
  std::size_t aRight = pLength - 1U;
  while (aLeft < aRight) {
    const unsigned char aTemp = pBuffer[aLeft];
    pBuffer[aLeft] = pBuffer[aRight];
    pBuffer[aRight] = aTemp;
    ++aLeft;
    --aRight;
  }
}

void InvertBytes(unsigned char* pBuffer, std::size_t pLength) {
  if (pBuffer == nullptr) {
    return;
  }
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    pBuffer[aIndex] = static_cast<unsigned char>(~pBuffer[aIndex]);
  }
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
    case TableFillKind::kAesCounter1:
    case TableFillKind::kAesCounter2:
    case TableFillKind::kAesCounter3: {
      if (Tables::gAesCounter == nullptr) {
        return false;
      }
      Tables::gAesCounter->Seed(pPassword, pPasswordLength);
      Tables::gAesCounter->Get(pDestination, static_cast<int>(pDestinationLength));
      if (pKind == TableFillKind::kAesCounter2) {
        ReverseBytes(pDestination, pDestinationLength);
      } else if (pKind == TableFillKind::kAesCounter3) {
        InvertBytes(pDestination, pDestinationLength);
      }
      return true;
    }
    case TableFillKind::kAriaCounter: {
      if (Tables::gAriaCounter == nullptr) {
        return false;
      }
      Tables::gAriaCounter->Seed(pPassword, pPasswordLength);
      Tables::gAriaCounter->Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case TableFillKind::kChaChaCounter: {
      if (Tables::gChaChaCounter == nullptr) {
        return false;
      }
      Tables::gChaChaCounter->Seed(pPassword, pPasswordLength);
      Tables::gChaChaCounter->Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    default:
      return false;
  }
}

bool FillExpander(TableFillKind pKind,
                  unsigned char* pPassword,
                  int pPasswordLength,
                  unsigned char* pDestination,
                  std::size_t pDestinationLength) {
  if (pDestination == nullptr) {
    return false;
  }

  PasswordExpander::ExpandPasswordBlocksByIndex(ExpanderIndexForKind(pKind),
                                                pPassword,
                                                (pPasswordLength > 0) ? static_cast<unsigned int>(pPasswordLength) : 0U,
                                                Tables::gExpanderWorker,
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

std::size_t GameStride(GameStyle pStyle) {
  return (pStyle == GameStyle::kSparse) ? 4U : 1U;
}

std::size_t MazeStride(MazeStyle pStyle) {
  return (pStyle == MazeStyle::kSparse) ? 4U : 1U;
}

bool ShouldCancel(const LaunchRequest& pRequest) {
  return pRequest.mShouldCancel != nullptr && pRequest.mShouldCancel(pRequest.mCancelUserData);
}

void SeedFastRand(const LaunchRequest& pRequest) {
  Tables::gFastRand.Seed(pRequest.mPassword, pRequest.mPasswordLength);
}

void ShuffleFillOrder() {
  for (std::size_t aIndex = 0; aIndex < Tables::kBaseFillKindCount; ++aIndex) {
    Tables::gFillOrder[aIndex] = kBaseFillKinds[aIndex];
  }

  for (std::size_t aIndex = 0; aIndex < Tables::kBaseFillKindCount; ++aIndex) {
    const std::size_t aRemaining = Tables::kBaseFillKindCount - aIndex;
    const std::size_t aSwapIndex = aIndex + static_cast<std::size_t>(Tables::gFastRand.Get(static_cast<int>(aRemaining)));
    const TableFillKind aTemp = Tables::gFillOrder[aIndex];
    Tables::gFillOrder[aIndex] = Tables::gFillOrder[aSwapIndex];
    Tables::gFillOrder[aSwapIndex] = aTemp;
  }

  for (std::size_t aIndex = Tables::kBaseFillKindCount; aIndex < Tables::kTableCount; ++aIndex) {
    Tables::gFillOrder[aIndex] =
        Tables::gFillOrder[static_cast<std::size_t>(Tables::gFastRand.Get(static_cast<int>(Tables::kBaseFillKindCount)))];
  }
}

bool FillTable(TableFillKind pKind,
               unsigned char* pPassword,
               int pPasswordLength,
               unsigned char* pDestination,
               std::size_t pDestinationLength) {
  if (FillCounter(pKind, pPassword, pPasswordLength, pDestination, pDestinationLength)) {
    return true;
  }
  return FillExpander(pKind, pPassword, pPasswordLength, pDestination, pDestinationLength);
}

bool FastFillTable(std::size_t pTableIndex,
                   TableFillKind pKind,
                   unsigned char* pDestination,
                   std::size_t pDestinationLength) {
  int aSeed = static_cast<int>(Tables::gFastRand.GetInt());
  aSeed ^= static_cast<int>((pTableIndex + 1U) * 257U);
  aSeed ^= static_cast<int>((static_cast<unsigned int>(pKind) + 1U) * 65537U);
  aSeed ^= static_cast<int>(pDestinationLength & 0x7FFFFFFFU);
  FastFill(pDestination, static_cast<int>(pDestinationLength), aSeed);
  return true;
}

std::string FillLabel(TableFillKind pKind) {
  switch (pKind) {
    case TableFillKind::kAesCounter1:
      return "aes_1";
    case TableFillKind::kAesCounter2:
      return "aes_2";
    case TableFillKind::kAesCounter3:
      return "aes_3";
    case TableFillKind::kAriaCounter:
      return "aria";
    case TableFillKind::kChaChaCounter:
      return "chacha";
    case TableFillKind::kPasswordExpander00:
      return "p_00";
    case TableFillKind::kPasswordExpander01:
      return "p_01";
    case TableFillKind::kPasswordExpander02:
      return "p_02";
    case TableFillKind::kPasswordExpander03:
      return "p_03";
    case TableFillKind::kPasswordExpander04:
      return "p_04";
    case TableFillKind::kPasswordExpander05:
      return "p_05";
    case TableFillKind::kPasswordExpander06:
      return "p_06";
    case TableFillKind::kPasswordExpander07:
      return "p_07";
    case TableFillKind::kPasswordExpander08:
      return "p_08";
    case TableFillKind::kPasswordExpander09:
      return "p_09";
    case TableFillKind::kPasswordExpander10:
      return "p_10";
    case TableFillKind::kPasswordExpander11:
      return "p_11";
    case TableFillKind::kPasswordExpander12:
      return "p_12";
    case TableFillKind::kPasswordExpander13:
      return "p_13";
    case TableFillKind::kPasswordExpander14:
      return "p_14";
    case TableFillKind::kPasswordExpander15:
      return "p_15";
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
  if (pStyle == GameStyle::kNone || (pTableSize % static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity)) != 0U) {
    return 0U;
  }
  const std::size_t aBlockCount =
      pTableSize / static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity);
  return CountProcessedBlocks(aBlockCount, GameStride(pStyle));
}

std::size_t CountMazeBlocks(std::size_t pTableSize, MazeStyle pStyle) {
  if (pStyle == MazeStyle::kNone || (pTableSize % static_cast<std::size_t>(peanutbutter::maze::MazeDirector::kSeedBufferCapacity)) != 0U) {
    return 0U;
  }
  const std::size_t aBlockCount =
      pTableSize / static_cast<std::size_t>(peanutbutter::maze::MazeDirector::kSeedBufferCapacity);
  return CountProcessedBlocks(aBlockCount, MazeStride(pStyle));
}

}  // namespace peanutbutter::tables::helpers
