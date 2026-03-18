#include "src/ArchiverCompatibility.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace peanutbutter::archiver {

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;
using peanutbutter::games::GameBoard;
using peanutbutter::maze::MazeDirector;

constexpr std::uint8_t kCurrentExpanderLibraryVersion = 1U;
constexpr std::array<int, 19> kReplayPattern = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 0, 1, 2,
};

struct GlobalTableDescriptor {
  const char* mName;
  unsigned char* mData;
  std::size_t mSize;
};

const GlobalTableDescriptor kGlobalTables[] = {
    {"L1_A", gTableL1_A, BLOCK_SIZE_L1}, {"L1_B", gTableL1_B, BLOCK_SIZE_L1}, {"L1_C", gTableL1_C, BLOCK_SIZE_L1},
    {"L1_D", gTableL1_D, BLOCK_SIZE_L1}, {"L1_E", gTableL1_E, BLOCK_SIZE_L1}, {"L1_F", gTableL1_F, BLOCK_SIZE_L1},
    {"L1_G", gTableL1_G, BLOCK_SIZE_L1}, {"L1_H", gTableL1_H, BLOCK_SIZE_L1}, {"L2_A", gTableL2_A, BLOCK_SIZE_L2},
    {"L2_B", gTableL2_B, BLOCK_SIZE_L2}, {"L2_C", gTableL2_C, BLOCK_SIZE_L2}, {"L2_D", gTableL2_D, BLOCK_SIZE_L2},
    {"L2_E", gTableL2_E, BLOCK_SIZE_L2}, {"L2_F", gTableL2_F, BLOCK_SIZE_L2}, {"L3_A", gTableL3_A, BLOCK_SIZE_L3},
    {"L3_B", gTableL3_B, BLOCK_SIZE_L3}, {"L3_C", gTableL3_C, BLOCK_SIZE_L3}, {"L3_D", gTableL3_D, BLOCK_SIZE_L3},
};

double ClampFraction(double pFraction) {
  if (pFraction < 0.0) {
    return 0.0;
  }
  if (pFraction > 1.0) {
    return 1.0;
  }
  return pFraction;
}

std::string BuildLogMessage(int pCode, const std::string& pMessage) {
  return "[Expansion][" + std::to_string(pCode) + "] " + pMessage;
}

void LogStatus(Logger* pLogger, int pCode, const std::string& pMessage) {
  if (pLogger != nullptr) {
    pLogger->LogStatus(BuildLogMessage(pCode, pMessage));
  }
}

void LogError(Logger* pLogger, int pCode, const std::string& pMessage) {
  if (pLogger != nullptr) {
    pLogger->LogError(BuildLogMessage(pCode, pMessage));
  }
}

std::string ResolveModeName(const char* pModeName) {
  if (pModeName == nullptr || *pModeName == '\0') {
    return "Bundle";
  }
  return std::string(pModeName);
}

bool ShouldCancel(const LaunchRequest& pRequest) {
  return pRequest.mShouldCancel != nullptr && pRequest.mShouldCancel(pRequest.mCancelUserData);
}

int ReplayPassCount(ExpansionStrength pStrength) {
  switch (pStrength) {
    case ExpansionStrength::kLow:
      return 1;
    case ExpansionStrength::kNormal:
      return 1;
    case ExpansionStrength::kHigh:
      return 2;
    case ExpansionStrength::kExtreme:
      return 3;
  }
  return 1;
}

bool FillCounterTable(const LaunchRequest& pRequest,
                      std::size_t pTableIndex,
                      unsigned char* pDestination,
                      std::size_t pDestinationLength) {
  if (pDestination == nullptr) {
    return false;
  }

  switch (pTableIndex) {
    case 0: {
      AESCounter aCounter;
      aCounter.Seed(pRequest.mPassword, pRequest.mPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case 1: {
      ChaCha20Counter aCounter;
      aCounter.Seed(pRequest.mPassword, pRequest.mPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    case 2: {
      ARIA256Counter aCounter;
      aCounter.Seed(pRequest.mPassword, pRequest.mPasswordLength);
      aCounter.Get(pDestination, static_cast<int>(pDestinationLength));
      return true;
    }
    default:
      return false;
  }
}

bool FillScratchTable(const LaunchRequest& pRequest,
                      const GlobalTableDescriptor& pDescriptor,
                      std::size_t pTableIndex,
                      unsigned char* pDestination) {
  if (pDestination == nullptr) {
    return false;
  }

  if (pTableIndex < 3U) {
    return FillCounterTable(pRequest, pTableIndex, pDestination, pDescriptor.mSize);
  }

  const unsigned char aType = static_cast<unsigned char>(pTableIndex - 3U);
  if (aType >= static_cast<unsigned char>(PasswordExpander::kTypeCount)) {
    return false;
  }

  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  PasswordExpander::ExpandPasswordBlocksByIndex(aType,
                                                pRequest.mPassword,
                                                (pRequest.mPasswordLength > 0)
                                                    ? static_cast<unsigned int>(pRequest.mPasswordLength)
                                                    : 0U,
                                                aWorker.data(),
                                                pDestination,
                                                static_cast<unsigned int>(pDescriptor.mSize));
  return true;
}

template <std::size_t kBlockSize>
bool TableSizeMatchesBlocks(std::size_t pTableSize) {
  return pTableSize > 0U && (pTableSize % kBlockSize) == 0U;
}

bool ApplyGamesToTable(const LaunchRequest& pRequest,
                       unsigned char* pTableBytes,
                       std::size_t pTableSize,
                       int pPassCount) {
  if (pTableBytes == nullptr || !TableSizeMatchesBlocks<GameBoard::kSeedBufferCapacity>(pTableSize)) {
    return false;
  }

  std::array<unsigned char, GameBoard::kSeedBufferCapacity> aOutput = {};
  GameBoard aBoard;
  const std::size_t aBlockCount = pTableSize / static_cast<std::size_t>(GameBoard::kSeedBufferCapacity);

  for (int aPassIndex = 0; aPassIndex < pPassCount; ++aPassIndex) {
    for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; ++aBlockIndex) {
      if (ShouldCancel(pRequest)) {
        return false;
      }

      const int aGameIndex = kReplayPattern[aBlockIndex % kReplayPattern.size()];
      unsigned char* const aBlock = pTableBytes + (aBlockIndex * static_cast<std::size_t>(GameBoard::kSeedBufferCapacity));
      aBoard.SetGame(aGameIndex);
      aBoard.Seed(aBlock, GameBoard::kSeedBufferCapacity);
      aBoard.Get(aOutput.data(), GameBoard::kSeedBufferCapacity);
      std::memcpy(aBlock, aOutput.data(), aOutput.size());
    }
  }

  return true;
}

bool ApplyMazesToTable(const LaunchRequest& pRequest,
                       unsigned char* pTableBytes,
                       std::size_t pTableSize,
                       int pPassCount) {
  if (pTableBytes == nullptr || !TableSizeMatchesBlocks<MazeDirector::kSeedBufferCapacity>(pTableSize)) {
    return false;
  }

  std::array<unsigned char, MazeDirector::kSeedBufferCapacity> aOutput = {};
  MazeDirector aMaze;
  const std::size_t aBlockCount = pTableSize / static_cast<std::size_t>(MazeDirector::kSeedBufferCapacity);

  for (int aPassIndex = 0; aPassIndex < pPassCount; ++aPassIndex) {
    for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; ++aBlockIndex) {
      if (ShouldCancel(pRequest)) {
        return false;
      }

      const int aMazeIndex = kReplayPattern[aBlockIndex % kReplayPattern.size()];
      unsigned char* const aBlock = pTableBytes + (aBlockIndex * static_cast<std::size_t>(MazeDirector::kSeedBufferCapacity));
      aMaze.SetGame(aMazeIndex);
      aMaze.Seed(aBlock, MazeDirector::kSeedBufferCapacity);
      aMaze.Get(aOutput.data(), MazeDirector::kSeedBufferCapacity);
      std::memcpy(aBlock, aOutput.data(), aOutput.size());
    }
  }

  return true;
}

}  // namespace

std::uint8_t ExpanderLibraryVersion() {
  return kCurrentExpanderLibraryVersion;
}

const char* ExpansionStrengthName(ExpansionStrength pStrength) {
  switch (pStrength) {
    case ExpansionStrength::kLow:
      return "low";
    case ExpansionStrength::kNormal:
      return "normal";
    case ExpansionStrength::kHigh:
      return "high";
    case ExpansionStrength::kExtreme:
      return "extreme";
  }
  return "unknown";
}

void ReportProgress(Logger& pLogger,
                    const std::string& pModeName,
                    ProgressProfileKind pProfile,
                    ProgressPhase pPhase,
                    double pPhaseFraction,
                    const std::string& pDetail) {
  (void)pProfile;
  ProgressInfo aInfo;
  aInfo.mModeName = pModeName;
  aInfo.mPhase = pPhase;
  aInfo.mOverallFraction = ClampFraction(pPhaseFraction);
  aInfo.mDetail = pDetail;
  pLogger.LogProgress(aInfo);
}

bool Launch(unsigned char* pPassword,
            int pPasswordLength,
            std::uint8_t pExpanderVersion,
            ExpansionStrength pExpansionStrength,
            Logger* pLogger,
            const char* pModeName,
            ProgressProfileKind pProgressProfile,
            ExpansionCancelFn pShouldCancel,
            void* pCancelUserData) {
  LaunchRequest aRequest;
  aRequest.mPassword = pPassword;
  aRequest.mPasswordLength = pPasswordLength;
  aRequest.mExpanderVersion = pExpanderVersion;
  aRequest.mExpansionStrength = pExpansionStrength;
  aRequest.mLogger = pLogger;
  aRequest.mModeName = pModeName;
  aRequest.mProgressProfile = pProgressProfile;
  aRequest.mShouldCancel = pShouldCancel;
  aRequest.mCancelUserData = pCancelUserData;
  return Launch(aRequest);
}

bool Launch(const LaunchRequest& pRequest) {
  if (pRequest.mPasswordLength < 0) {
    LogError(pRequest.mLogger, 180, "Expansion failed: invalid password length.");
    return false;
  }
  if (pRequest.mPasswordLength > 0 && pRequest.mPassword == nullptr) {
    LogError(pRequest.mLogger, 181, "Expansion failed: password bytes were missing.");
    return false;
  }

  const std::string aModeName = ResolveModeName(pRequest.mModeName);
  LogStatus(pRequest.mLogger, 100, "Starting password expansion.");
  LogStatus(pRequest.mLogger,
            101,
            "Using expander library version " + std::to_string(ExpanderLibraryVersion()) +
                " with strength=" + ExpansionStrengthName(pRequest.mExpansionStrength) + ".");

  if (pRequest.mExpanderVersion != ExpanderLibraryVersion()) {
    LogStatus(pRequest.mLogger,
              161,
              "Warning: expander library version mismatch; continuing with local implementation.");
  }

  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   aModeName,
                   pRequest.mProgressProfile,
                   ProgressPhase::kExpansion,
                   0.0,
                   "Starting password expansion.");
  }

  if (ShouldCancel(pRequest)) {
    LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
    return false;
  }

  const int aReplayPassCount = ReplayPassCount(pRequest.mExpansionStrength);
  const std::size_t aTableCount = sizeof(kGlobalTables) / sizeof(kGlobalTables[0]);
  const std::size_t aProgressUnitCount = aTableCount * static_cast<std::size_t>(1 + aReplayPassCount + aReplayPassCount);
  std::size_t aProgressUnitsComplete = 0U;

  std::vector<std::vector<unsigned char>> aScratchTables(sizeof(kGlobalTables) / sizeof(kGlobalTables[0]));

  for (std::size_t aTableIndex = 0; aTableIndex < aTableCount; ++aTableIndex) {
    if (ShouldCancel(pRequest)) {
      LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
      return false;
    }

    const GlobalTableDescriptor& aDescriptor = kGlobalTables[aTableIndex];
    aScratchTables[aTableIndex].assign(aDescriptor.mSize, 0U);
    if (!FillScratchTable(pRequest, aDescriptor, aTableIndex, aScratchTables[aTableIndex].data())) {
      LogError(pRequest.mLogger, 182, "Expansion failed while generating table memory.");
      return false;
    }

    if (pRequest.mLogger != nullptr) {
      ++aProgressUnitsComplete;
      const double aFraction =
          static_cast<double>(aProgressUnitsComplete) / static_cast<double>(aProgressUnitCount);
      ReportProgress(*pRequest.mLogger,
                     aModeName,
                     pRequest.mProgressProfile,
                     ProgressPhase::kExpansion,
                     aFraction,
                     std::string("Filled table ") + aDescriptor.mName + ".");
    }
  }

  for (int aPassIndex = 0; aPassIndex < aReplayPassCount; ++aPassIndex) {
    for (std::size_t aTableIndex = 0; aTableIndex < aTableCount; ++aTableIndex) {
      if (ShouldCancel(pRequest)) {
        LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
        return false;
      }

      const GlobalTableDescriptor& aDescriptor = kGlobalTables[aTableIndex];
      if (!ApplyGamesToTable(pRequest, aScratchTables[aTableIndex].data(), aDescriptor.mSize, 1)) {
        if (ShouldCancel(pRequest)) {
          LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
        } else {
          LogError(pRequest.mLogger, 183, "Expansion failed while applying game shufflers.");
        }
        return false;
      }

      if (pRequest.mLogger != nullptr) {
        ++aProgressUnitsComplete;
        const double aFraction =
            static_cast<double>(aProgressUnitsComplete) / static_cast<double>(aProgressUnitCount);
        ReportProgress(*pRequest.mLogger,
                       aModeName,
                       pRequest.mProgressProfile,
                       ProgressPhase::kExpansion,
                       aFraction,
                       std::string("Applied game pass ") + std::to_string(aPassIndex + 1) + "/" +
                           std::to_string(aReplayPassCount) + " to " + aDescriptor.mName + ".");
      }
    }
  }

  for (int aPassIndex = 0; aPassIndex < aReplayPassCount; ++aPassIndex) {
    for (std::size_t aTableIndex = 0; aTableIndex < aTableCount; ++aTableIndex) {
      if (ShouldCancel(pRequest)) {
        LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
        return false;
      }

      const GlobalTableDescriptor& aDescriptor = kGlobalTables[aTableIndex];
      if (!ApplyMazesToTable(pRequest, aScratchTables[aTableIndex].data(), aDescriptor.mSize, 1)) {
        if (ShouldCancel(pRequest)) {
          LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
        } else {
          LogError(pRequest.mLogger, 184, "Expansion failed while applying maze shufflers.");
        }
        return false;
      }

      if (pRequest.mLogger != nullptr) {
        ++aProgressUnitsComplete;
        const double aFraction =
            static_cast<double>(aProgressUnitsComplete) / static_cast<double>(aProgressUnitCount);
        ReportProgress(*pRequest.mLogger,
                       aModeName,
                       pRequest.mProgressProfile,
                       ProgressPhase::kExpansion,
                       aFraction,
                       std::string("Applied maze pass ") + std::to_string(aPassIndex + 1) + "/" +
                           std::to_string(aReplayPassCount) + " to " + aDescriptor.mName + ".");
      }
    }
  }

  if (ShouldCancel(pRequest)) {
    LogStatus(pRequest.mLogger, 170, "Password expansion canceled.");
    return false;
  }

  for (std::size_t aTableIndex = 0; aTableIndex < (sizeof(kGlobalTables) / sizeof(kGlobalTables[0])); ++aTableIndex) {
    std::memcpy(kGlobalTables[aTableIndex].mData,
                aScratchTables[aTableIndex].data(),
                kGlobalTables[aTableIndex].mSize);
  }

  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   aModeName,
                   pRequest.mProgressProfile,
                   ProgressPhase::kExpansion,
                   1.0,
                   "Password expansion complete.");
  }

  LogStatus(pRequest.mLogger, 140, "Password expansion complete.");
  return true;
}

}  // namespace peanutbutter::archiver
