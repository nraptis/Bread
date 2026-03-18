#include "src/Tables/Tables.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "src/Tables/TablesHelpers.hpp"
#include "src/Tables/TablesTiming.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"

namespace peanutbutter::tables {

std::uint8_t ExpanderLibraryVersion() {
  return 1U;
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

const char* GameStyleName(GameStyle pStyle) {
  switch (pStyle) {
    case GameStyle::kNone:
      return "none";
    case GameStyle::kSparse:
      return "sparse";
    case GameStyle::kFull:
      return "full";
  }
  return "unknown";
}

const char* MazeStyleName(MazeStyle pStyle) {
  switch (pStyle) {
    case MazeStyle::kNone:
      return "none";
    case MazeStyle::kSparse:
      return "sparse";
    case MazeStyle::kFull:
      return "full";
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
  aInfo.mOverallFraction = helpers::ClampFraction(pPhaseFraction);
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
  return Tables::Launch(pRequest);
}

unsigned char Tables::gTableL1_A[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_B[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_C[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_D[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_E[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_F[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_G[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_H[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_I[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_J[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_K[BLOCK_SIZE_L1] = {};
unsigned char Tables::gTableL1_L[BLOCK_SIZE_L1] = {};

unsigned char Tables::gTableL2_A[BLOCK_SIZE_L2] = {};
unsigned char Tables::gTableL2_B[BLOCK_SIZE_L2] = {};
unsigned char Tables::gTableL2_C[BLOCK_SIZE_L2] = {};
unsigned char Tables::gTableL2_D[BLOCK_SIZE_L2] = {};
unsigned char Tables::gTableL2_E[BLOCK_SIZE_L2] = {};
unsigned char Tables::gTableL2_F[BLOCK_SIZE_L2] = {};

unsigned char Tables::gTableL3_A[BLOCK_SIZE_L3] = {};
unsigned char Tables::gTableL3_B[BLOCK_SIZE_L3] = {};
unsigned char Tables::gTableL3_C[BLOCK_SIZE_L3] = {};
unsigned char Tables::gTableL3_D[BLOCK_SIZE_L3] = {};

unsigned char Tables::mScratch[Tables::kScratchSize] = {};
int Tables::mRandomTableIndex = 0;
int Tables::mRandomByteIndex = static_cast<int>(BLOCK_SIZE_L1) - 1;
int Tables::mPlayOrder[16] = {};

const std::array<Tables::TableDescriptor, Tables::kTableCount>& Tables::All() {
  static const std::array<TableDescriptor, kTableCount> kAllTables = {{
      {"L1_A", gTableL1_A, BLOCK_SIZE_L1}, {"L1_B", gTableL1_B, BLOCK_SIZE_L1}, {"L1_C", gTableL1_C, BLOCK_SIZE_L1},
      {"L1_D", gTableL1_D, BLOCK_SIZE_L1}, {"L1_E", gTableL1_E, BLOCK_SIZE_L1}, {"L1_F", gTableL1_F, BLOCK_SIZE_L1},
      {"L1_G", gTableL1_G, BLOCK_SIZE_L1}, {"L1_H", gTableL1_H, BLOCK_SIZE_L1}, {"L1_I", gTableL1_I, BLOCK_SIZE_L1},
      {"L1_J", gTableL1_J, BLOCK_SIZE_L1}, {"L1_K", gTableL1_K, BLOCK_SIZE_L1}, {"L1_L", gTableL1_L, BLOCK_SIZE_L1},
      {"L2_A", gTableL2_A, BLOCK_SIZE_L2}, {"L2_B", gTableL2_B, BLOCK_SIZE_L2}, {"L2_C", gTableL2_C, BLOCK_SIZE_L2},
      {"L2_D", gTableL2_D, BLOCK_SIZE_L2}, {"L2_E", gTableL2_E, BLOCK_SIZE_L2}, {"L2_F", gTableL2_F, BLOCK_SIZE_L2},
      {"L3_A", gTableL3_A, BLOCK_SIZE_L3}, {"L3_B", gTableL3_B, BLOCK_SIZE_L3}, {"L3_C", gTableL3_C, BLOCK_SIZE_L3},
      {"L3_D", gTableL3_D, BLOCK_SIZE_L3},
  }};
  return kAllTables;
}

void Tables::ResetRandomCursor() {
  mRandomTableIndex = 0;
  mRandomByteIndex = static_cast<int>(BLOCK_SIZE_L1) - 1;
}

unsigned char Tables::Get() {
  ++mRandomTableIndex;
  if (mRandomTableIndex >= static_cast<int>(kTableCount)) {
    mRandomTableIndex = 0;
    --mRandomByteIndex;
    if (mRandomByteIndex <= 0) {
      mRandomByteIndex = static_cast<int>(BLOCK_SIZE_L1) - 1;
    }
  }

  const TableDescriptor& aTable = All()[static_cast<std::size_t>(mRandomTableIndex)];
  return aTable.mData[mRandomByteIndex];
}

unsigned char Tables::Get(int pMax) {
  if (pMax <= 0) {
    return 0U;
  }
  return static_cast<unsigned char>(static_cast<int>(Get()) % pMax);
}

void Tables::ShufflePlayOrder() {
  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    mPlayOrder[aIndex] = aIndex;
  }

  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    const int aWhich = static_cast<int>(gTableL1_A[aIndex % static_cast<int>(BLOCK_SIZE_L1)] % 16U);
    const int aTemp = mPlayOrder[aIndex];
    mPlayOrder[aIndex] = mPlayOrder[aWhich];
    mPlayOrder[aWhich] = aTemp;
  }
}

bool Tables::Launch(const LaunchRequest& pRequest) {
  if (pRequest.mPasswordLength < 0) {
    helpers::LogError(pRequest.mLogger, "Expansion failed: invalid password length.");
    return false;
  }
  if (pRequest.mPasswordLength > 0 && pRequest.mPassword == nullptr) {
    helpers::LogError(pRequest.mLogger, "Expansion failed: password bytes were missing.");
    return false;
  }

  const std::string aModeName = helpers::ResolveModeName(pRequest.mModeName);
  helpers::LogStatus(pRequest.mLogger, "Password expansion and table generation has started.");
  helpers::LogStatus(pRequest.mLogger,
                     "Expanding passwords across " + std::to_string(kL1TableCount) + " L1 buffers.");
  helpers::LogStatus(pRequest.mLogger,
                     "Expanding passwords across " + std::to_string(kL2TableCount) + " L2 buffers.");
  helpers::LogStatus(pRequest.mLogger,
                     "Expanding passwords across " + std::to_string(kL3TableCount) + " L3 buffers.");
  helpers::LogStatus(pRequest.mLogger, "Seeding 3 buffers with AES counter.");
  helpers::LogStatus(pRequest.mLogger, "Seeding 1 buffer with Aria counter.");
  helpers::LogStatus(pRequest.mLogger, "Seeding 1 buffer with ChaCha20 counter.");
  helpers::LogStatus(pRequest.mLogger, "Seeding 16 buffers with A.I. generated counters 1-16.");
  helpers::LogStatus(pRequest.mLogger, "Seeded 22 buffers.");
  


  if (pRequest.mExpanderVersion != ExpanderLibraryVersion()) {
    helpers::LogStatus(pRequest.mLogger,
                       "Warning: expander library version mismatch; continuing with local implementation.");
  }

  if (helpers::ShouldCancel(pRequest)) {
    helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
    return false;
  }

  helpers::BuildScratch(pRequest);
  const auto aPlan = helpers::BuildTablePlan();
  const auto& aTables = All();
  const timing::WorkEstimate aWork = timing::BuildWorkEstimate(pRequest.mGameStyle,
                                                               pRequest.mMazeStyle,
                                                               pRequest.mIsFastMode);
  std::vector<std::vector<unsigned char>> aScratchTables(kTableCount);
  double aCompletedMilliseconds = 0.0;

  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   aModeName,
                   pRequest.mProgressProfile,
                   ProgressPhase::kExpansion,
                   0.0,
                   "Starting table generation.");
  }

  for (std::size_t aIndex = 0; aIndex < kTableCount; ++aIndex) {
    if (helpers::ShouldCancel(pRequest)) {
      helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
      return false;
    }

    aScratchTables[aIndex].assign(aTables[aIndex].mSize, 0U);
    unsigned char* const aDestination = aScratchTables[aIndex].data();
    const bool aFillOk = pRequest.mIsFastMode
                             ? helpers::FastFillTable(aIndex, aPlan[aIndex], aDestination, aTables[aIndex].mSize)
                             : helpers::FillTable(aPlan[aIndex],
                                                  pRequest.mPassword,
                                                  pRequest.mPasswordLength,
                                                  aDestination,
                                                  aTables[aIndex].mSize);
    if (!aFillOk) {
      helpers::LogError(pRequest.mLogger, "Expansion failed while generating table memory.");
      return false;
    }

    aCompletedMilliseconds += timing::EstimateSeedMilliseconds(aTables[aIndex].mSize, pRequest.mIsFastMode);
    if (pRequest.mLogger != nullptr) {
      const double aFraction = aCompletedMilliseconds / aWork.mTotalMilliseconds;
      ReportProgress(*pRequest.mLogger,
                     aModeName,
                     pRequest.mProgressProfile,
                     ProgressPhase::kExpansion,
                     aFraction,
                     std::string("Filled table ") + aTables[aIndex].mName + " with " +
                         (pRequest.mIsFastMode ? std::string("fastfill") : helpers::FillLabel(aPlan[aIndex])) + ".");
    }
  }

  helpers::ShufflePlayOrderFromBuffer(aScratchTables[0].data(), aScratchTables[0].size(), mPlayOrder, 16);

  if (pRequest.mIsFastMode || pRequest.mGameStyle == GameStyle::kNone) {
    helpers::LogStatus(pRequest.mLogger, "Skipping match-three game play confusion.");
  } else {
    helpers::LogStatus(pRequest.mLogger,
                       "Appling game play confusion with " + std::to_string(games::GameBoard::kGameCount) +
                           " match-three puzzle games.");
    std::array<unsigned char, games::GameBoard::kSeedBufferCapacity> aOutput = {};
    games::GameBoard aBoard;
    const std::size_t aStride = helpers::GameStride(pRequest.mGameStyle);
    std::size_t aProcessedGameBlocks = 0U;
    for (std::size_t aTableIndex = 0; aTableIndex < kTableCount; ++aTableIndex) {
      if ((aTables[aTableIndex].mSize % static_cast<std::size_t>(games::GameBoard::kSeedBufferCapacity)) != 0U) {
        continue;
      }
      const std::size_t aBlockCount =
          aTables[aTableIndex].mSize / static_cast<std::size_t>(games::GameBoard::kSeedBufferCapacity);
      std::size_t aProcessedIndex = 0U;
      for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
        if (helpers::ShouldCancel(pRequest)) {
          helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
          return false;
        }
        const int aGameIndex = mPlayOrder[aProcessedIndex % 16U];
        unsigned char* const aBlock =
            aScratchTables[aTableIndex].data() +
            (aBlockIndex * static_cast<std::size_t>(games::GameBoard::kSeedBufferCapacity));
        aBoard.SetGame(aGameIndex);
        aBoard.Seed(aBlock, games::GameBoard::kSeedBufferCapacity);
        aBoard.Get(aOutput.data(), games::GameBoard::kSeedBufferCapacity);
        std::memcpy(aBlock, aOutput.data(), aOutput.size());
        ++aProcessedIndex;
        ++aProcessedGameBlocks;

        aCompletedMilliseconds += timing::ActiveTimingProfile().mGameBlockMilliseconds;
        if (pRequest.mLogger != nullptr) {
          const double aFraction = aCompletedMilliseconds / aWork.mTotalMilliseconds;
          ReportProgress(*pRequest.mLogger,
                         aModeName,
                         pRequest.mProgressProfile,
                         ProgressPhase::kExpansion,
                         aFraction,
                         "Applied match-three game play confusion block " + std::to_string(aProcessedGameBlocks) +
                             " of " + std::to_string(aWork.mGameBlockCount) + ".");
        }
      }
    }
  }

  if (pRequest.mIsFastMode || pRequest.mMazeStyle == MazeStyle::kNone) {
    helpers::LogStatus(pRequest.mLogger, "Skipping maze game play confusion.");
  } else {
    helpers::LogStatus(pRequest.mLogger,
                       "Appling maze game play confusion with " + std::to_string(maze::MazeDirector::kGameCount) +
                           " maze simulations.");
    std::array<unsigned char, maze::MazeDirector::kSeedBufferCapacity> aOutput = {};
    maze::MazeDirector aMaze;
    const std::size_t aStride = helpers::MazeStride(pRequest.mMazeStyle);
    std::size_t aProcessedMazeBlocks = 0U;
    for (std::size_t aTableIndex = 0; aTableIndex < kTableCount; ++aTableIndex) {
      if ((aTables[aTableIndex].mSize % static_cast<std::size_t>(maze::MazeDirector::kSeedBufferCapacity)) != 0U) {
        continue;
      }
      const std::size_t aBlockCount =
          aTables[aTableIndex].mSize / static_cast<std::size_t>(maze::MazeDirector::kSeedBufferCapacity);
      std::size_t aProcessedIndex = 0U;
      for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
        if (helpers::ShouldCancel(pRequest)) {
          helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
          return false;
        }
        const int aMazeIndex = mPlayOrder[aProcessedIndex % 16U];
        unsigned char* const aBlock =
            aScratchTables[aTableIndex].data() +
            (aBlockIndex * static_cast<std::size_t>(maze::MazeDirector::kSeedBufferCapacity));
        aMaze.SetGame(aMazeIndex);
        aMaze.Seed(aBlock, maze::MazeDirector::kSeedBufferCapacity);
        aMaze.Get(aOutput.data(), maze::MazeDirector::kSeedBufferCapacity);
        std::memcpy(aBlock, aOutput.data(), aOutput.size());
        ++aProcessedIndex;
        ++aProcessedMazeBlocks;

        aCompletedMilliseconds += timing::ActiveTimingProfile().mMazeBlockMilliseconds;
        if (pRequest.mLogger != nullptr) {
          const double aFraction = aCompletedMilliseconds / aWork.mTotalMilliseconds;
          ReportProgress(*pRequest.mLogger,
                         aModeName,
                         pRequest.mProgressProfile,
                         ProgressPhase::kExpansion,
                         aFraction,
                         "Applied maze game play confusion block " + std::to_string(aProcessedMazeBlocks) +
                             " of " + std::to_string(aWork.mMazeBlockCount) + ".");
        }
      }
    }
  }

  helpers::LogStatus(pRequest.mLogger, "Finalizing tables...");
  for (std::size_t aIndex = 0; aIndex < kTableCount; ++aIndex) {
    std::memcpy(aTables[aIndex].mData, aScratchTables[aIndex].data(), aTables[aIndex].mSize);
  }
  if (pRequest.mLogger != nullptr) {
    aCompletedMilliseconds += aWork.mFinalizeMilliseconds;
    ReportProgress(*pRequest.mLogger,
                   aModeName,
                   pRequest.mProgressProfile,
                   ProgressPhase::kFinalize,
                   aCompletedMilliseconds / aWork.mTotalMilliseconds,
                   "Finalizing tables.");
  }
  ResetRandomCursor();
  helpers::LogStatus(pRequest.mLogger, "Password expansion and table generation has completed.");
  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   aModeName,
                   pRequest.mProgressProfile,
                   ProgressPhase::kFinalize,
                   1.0,
                   "Tables are ready.");
  }
  return true;
}

}  // namespace peanutbutter::tables
