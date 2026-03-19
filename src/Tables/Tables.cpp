#include "src/Tables/Tables.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "src/Tables/TablesHelpers.hpp"
#include "src/Tables/TablesTiming.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"

namespace peanutbutter::tables {

namespace {

constexpr const char* kDefaultModeName = "Bundle";

struct PuzzleGameSummary {
  const char* mName = nullptr;
  std::uint64_t mCompletedGames = 0U;
  std::uint64_t mMatchedTiles = 0U;
  std::uint64_t mPowerUpsCollected = 0U;
  std::uint64_t mCascadesTriggered = 0U;
  std::uint64_t mDragonAttacks = 0U;
  std::uint64_t mPhoenixAttacks = 0U;
  std::uint64_t mWyvernAttacks = 0U;
};

struct MazeSummary {
  std::uint64_t mCompletedRounds = 0U;
  std::uint64_t mCheeseCollected = 0U;
  std::uint64_t mRobotVictories = 0U;
  std::uint64_t mRobotDeaths = 0U;
  std::uint64_t mDolphinDeaths = 0U;
  std::uint64_t mDolphinCheeseSteals = 0U;
  std::uint64_t mStarBursts = 0U;
  std::uint64_t mChaosStorms = 0U;
  std::uint64_t mCometTrails = 0U;
};

std::string MazeSimulationName(int pGameIndex) {
  char aSuffix = static_cast<char>('A' + (pGameIndex % 26));
  return std::string("Maze-") + aSuffix;
}

void LogPuzzleGameSummaries(Logger* pLogger,
                            const std::array<PuzzleGameSummary, peanutbutter::games::GameBoard::kGameCount>& pSummaries) {
  std::uint64_t aDragonAttacks = 0U;
  std::uint64_t aPhoenixAttacks = 0U;
  std::uint64_t aWyvernAttacks = 0U;
  for (const PuzzleGameSummary& aSummary : pSummaries) {
    aDragonAttacks += aSummary.mDragonAttacks;
    aPhoenixAttacks += aSummary.mPhoenixAttacks;
    aWyvernAttacks += aSummary.mWyvernAttacks;
    if (aSummary.mCompletedGames == 0U || aSummary.mName == nullptr) {
      continue;
    }
    helpers::LogStatus(pLogger,
                       "Completed " + std::to_string(aSummary.mCompletedGames) + " games of " + aSummary.mName +
                           " puzzle game, matching " + std::to_string(aSummary.mMatchedTiles) + " tiles.");
    helpers::LogStatus(pLogger,
                       "Player collected " + std::to_string(aSummary.mPowerUpsCollected) + " power ups, triggered " +
                           std::to_string(aSummary.mCascadesTriggered) + " cascades.");
  }
  helpers::LogStatus(pLogger,
                     "Puzzle game rare events: " + std::to_string(aDragonAttacks) + " dragon attacks, " +
                         std::to_string(aPhoenixAttacks) + " phoenix attacks, " +
                         std::to_string(aWyvernAttacks) + " wyvern attacks.");
}

void LogMazeSummaries(Logger* pLogger, const std::array<MazeSummary, peanutbutter::maze::MazeDirector::kGameCount>& pSummaries) {
  std::uint64_t aStarBursts = 0U;
  std::uint64_t aChaosStorms = 0U;
  std::uint64_t aCometTrails = 0U;
  for (int aGameIndex = 0; aGameIndex < peanutbutter::maze::MazeDirector::kGameCount; ++aGameIndex) {
    const MazeSummary& aSummary = pSummaries[static_cast<std::size_t>(aGameIndex)];
    aStarBursts += aSummary.mStarBursts;
    aChaosStorms += aSummary.mChaosStorms;
    aCometTrails += aSummary.mCometTrails;
    if (aSummary.mCompletedRounds == 0U) {
      continue;
    }
    helpers::LogStatus(pLogger,
                       "Completed " + std::to_string(aSummary.mCompletedRounds) + " rounds of " +
                           MazeSimulationName(aGameIndex) + " simulation, collecting " +
                           std::to_string(aSummary.mCheeseCollected) + " pieces of cheese.");
    helpers::LogStatus(pLogger,
                       std::to_string(aSummary.mRobotVictories) + " robot victories, " +
                           std::to_string(aSummary.mRobotDeaths) + " robot deaths, " +
                           std::to_string(aSummary.mDolphinDeaths) + " dolphin deaths, " +
                           std::to_string(aSummary.mDolphinCheeseSteals) + " dolphin cheese steals.");
  }
  helpers::LogStatus(pLogger,
                     "Maze simulation rare events: " + std::to_string(aStarBursts) + " star bursts, " +
                         std::to_string(aChaosStorms) + " chaos storms, " + std::to_string(aCometTrails) +
                         " comet trails.");
}

}  // namespace

std::uint8_t ExpanderLibraryVersion() {
  return 1U;
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
            Logger* pLogger,
            const char* pModeName,
            ProgressProfileKind pProgressProfile,
            ExpansionCancelFn pShouldCancel,
            void* pCancelUserData) {
  (void)pModeName;
  (void)pProgressProfile;

  LaunchRequest aRequest;
  aRequest.mPassword = pPassword;
  aRequest.mPasswordLength = pPasswordLength;
  aRequest.mExpanderVersion = pExpanderVersion;
  aRequest.mLogger = pLogger;
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

peanutbutter::fast_rand::FastRand Tables::gFastRand = {};
TableFillKind Tables::gFillOrder[Tables::kTableCount] = {};
unsigned char Tables::gExpanderWorker[PASSWORD_EXPANDED_SIZE] = {};
int Tables::mRandomTableIndex = 0;
int Tables::mRandomByteIndex = static_cast<int>(BLOCK_SIZE_L1) - 1;
AESCounter* Tables::gAesCounter = nullptr;
ARIA256Counter* Tables::gAriaCounter = nullptr;
ChaCha20Counter* Tables::gChaChaCounter = nullptr;
peanutbutter::games::GameBoard* Tables::gGameBoard = nullptr;
peanutbutter::maze::MazeDirector* Tables::gMazeDirector = nullptr;

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

void Tables::EnsureRuntimeObjects() {
  if (gAesCounter == nullptr) {
    gAesCounter = new AESCounter();
  }
  if (gAriaCounter == nullptr) {
    gAriaCounter = new ARIA256Counter();
  }
  if (gChaChaCounter == nullptr) {
    gChaChaCounter = new ChaCha20Counter();
  }
  if (gGameBoard == nullptr) {
    gGameBoard = new peanutbutter::games::GameBoard();
  }
  if (gMazeDirector == nullptr) {
    gMazeDirector = new peanutbutter::maze::MazeDirector();
  }
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

bool Tables::Launch(const LaunchRequest& pRequest) {
  if (pRequest.mPasswordLength < 0) {
    helpers::LogError(pRequest.mLogger, "Expansion failed: invalid password length.");
    return false;
  }
  if (pRequest.mPasswordLength > 0 && pRequest.mPassword == nullptr) {
    helpers::LogError(pRequest.mLogger, "Expansion failed: password bytes were missing.");
    return false;
  }

  helpers::LogStatus(pRequest.mLogger, "Password expansion and table generation has started.");
  helpers::LogStatus(pRequest.mLogger,
                     "Generating " + std::to_string(kTableCount) + " table buffers for the release bundle.");

  if (pRequest.mExpanderVersion != ExpanderLibraryVersion()) {
    helpers::LogStatus(pRequest.mLogger,
                       "Warning: expander library version mismatch; continuing with local implementation.");
  }

  if (helpers::ShouldCancel(pRequest)) {
    helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
    return false;
  }

  EnsureRuntimeObjects();
  helpers::SeedFastRand(pRequest);
  helpers::ShuffleFillOrder();

  const auto& aTables = All();
  const timing::WorkEstimate aWork = timing::BuildWorkEstimate(pRequest.mGameStyle,
                                                               pRequest.mMazeStyle,
                                                               pRequest.mIsFastMode);
  double aCompletedMilliseconds = 0.0;

  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   kDefaultModeName,
                   ProgressProfileKind::kBundle,
                   ProgressPhase::kExpansion,
                   0.0,
                   "Starting table generation.");
  }

  for (std::size_t aIndex = 0; aIndex < kTableCount; ++aIndex) {
    if (helpers::ShouldCancel(pRequest)) {
      helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
      return false;
    }

    const bool aFillOk = pRequest.mIsFastMode
                             ? helpers::FastFillTable(aIndex, gFillOrder[aIndex], aTables[aIndex].mData, aTables[aIndex].mSize)
                             : helpers::FillTable(gFillOrder[aIndex],
                                                  pRequest.mPassword,
                                                  pRequest.mPasswordLength,
                                                  aTables[aIndex].mData,
                                                  aTables[aIndex].mSize);
    if (!aFillOk) {
      helpers::LogError(pRequest.mLogger, "Expansion failed while generating table memory.");
      return false;
    }

    aCompletedMilliseconds += timing::EstimateSeedMilliseconds(aTables[aIndex].mSize, pRequest.mIsFastMode);
  }

  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   kDefaultModeName,
                   ProgressProfileKind::kBundle,
                   ProgressPhase::kExpansion,
                   aCompletedMilliseconds / aWork.mTotalMilliseconds,
                   "All buffers seeded.");
  }

  if (pRequest.mIsFastMode || pRequest.mGameStyle == GameStyle::kNone) {
    helpers::LogStatus(pRequest.mLogger, "Skipping puzzle simulation stage.");
  } else {
    std::array<PuzzleGameSummary, peanutbutter::games::GameBoard::kGameCount> aPuzzleSummaries = {};
    helpers::LogStatus(pRequest.mLogger,
                       "Running puzzle simulation stage with " +
                           std::to_string(peanutbutter::games::GameBoard::kGameCount) + " match-three puzzle games.");
    const std::size_t aStride = helpers::GameStride(pRequest.mGameStyle);
    for (std::size_t aTableIndex = 0; aTableIndex < kTableCount; ++aTableIndex) {
      if ((aTables[aTableIndex].mSize % static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity)) != 0U) {
        continue;
      }
      const std::size_t aBlockCount =
          aTables[aTableIndex].mSize / static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity);
      for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
        if (helpers::ShouldCancel(pRequest)) {
          helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
          return false;
        }

        unsigned char* const aBlock =
            aTables[aTableIndex].mData +
            (aBlockIndex * static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity));
        const int aGameIndex = static_cast<int>(gFastRand.Get(peanutbutter::games::GameBoard::kGameCount));
        gGameBoard->SetGame(aGameIndex);
        gGameBoard->Seed(aBlock, peanutbutter::games::GameBoard::kSeedBufferCapacity);
        gGameBoard->Get(aBlock, peanutbutter::games::GameBoard::kSeedBufferCapacity);
        const peanutbutter::games::GameBoard::RuntimeStats aStats = gGameBoard->GetRuntimeStats();
        PuzzleGameSummary& aSummary = aPuzzleSummaries[static_cast<std::size_t>(aGameIndex)];
        aSummary.mName = gGameBoard->GetName();
        ++aSummary.mCompletedGames;
        aSummary.mMatchedTiles += aStats.mUserMatch + aStats.mCascadeMatch;
        aSummary.mPowerUpsCollected += aStats.mPowerUpConsumed;
        aSummary.mCascadesTriggered += aStats.mCascadeMatch;
        aSummary.mDragonAttacks += aStats.mDragonAttack;
        aSummary.mPhoenixAttacks += aStats.mPhoenixAttack;
        aSummary.mWyvernAttacks += aStats.mWyvernAttack;
        aCompletedMilliseconds += timing::ActiveTimingProfile().mGameBlockMilliseconds;
      }
    }
    LogPuzzleGameSummaries(pRequest.mLogger, aPuzzleSummaries);

    if (pRequest.mLogger != nullptr) {
      ReportProgress(*pRequest.mLogger,
                     kDefaultModeName,
                     ProgressProfileKind::kBundle,
                     ProgressPhase::kExpansion,
                     aCompletedMilliseconds / aWork.mTotalMilliseconds,
                     "Puzzle simulation stage complete.");
    }
  }

  if (pRequest.mIsFastMode || pRequest.mMazeStyle == MazeStyle::kNone) {
    helpers::LogStatus(pRequest.mLogger, "Skipping maze simulation stage.");
  } else {
    std::array<MazeSummary, peanutbutter::maze::MazeDirector::kGameCount> aMazeSummaries = {};
    helpers::LogStatus(pRequest.mLogger,
                       "Running maze simulation stage with " +
                           std::to_string(peanutbutter::maze::MazeDirector::kGameCount) + " maze simulations.");
    const std::size_t aStride = helpers::MazeStride(pRequest.mMazeStyle);
    for (std::size_t aTableIndex = 0; aTableIndex < kTableCount; ++aTableIndex) {
      if ((aTables[aTableIndex].mSize % static_cast<std::size_t>(peanutbutter::maze::MazeDirector::kSeedBufferCapacity)) != 0U) {
        continue;
      }
      const std::size_t aBlockCount =
          aTables[aTableIndex].mSize / static_cast<std::size_t>(peanutbutter::maze::MazeDirector::kSeedBufferCapacity);
      for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
        if (helpers::ShouldCancel(pRequest)) {
          helpers::LogStatus(pRequest.mLogger, "Table generation canceled.");
          return false;
        }

        unsigned char* const aBlock =
            aTables[aTableIndex].mData +
            (aBlockIndex * static_cast<std::size_t>(peanutbutter::maze::MazeDirector::kSeedBufferCapacity));
        const int aGameIndex = static_cast<int>(gFastRand.Get(peanutbutter::maze::MazeDirector::kGameCount));
        gMazeDirector->SetGame(aGameIndex);
        gMazeDirector->Seed(aBlock, peanutbutter::maze::MazeDirector::kSeedBufferCapacity);
        gMazeDirector->Get(aBlock, peanutbutter::maze::MazeDirector::kSeedBufferCapacity);
        const peanutbutter::maze::Maze::RuntimeStats aStats = gMazeDirector->GetRuntimeStats();
        MazeSummary& aSummary = aMazeSummaries[static_cast<std::size_t>(aGameIndex)];
        ++aSummary.mCompletedRounds;
        aSummary.mCheeseCollected += aStats.mVictories + aStats.mDolphinCheeseTriages;
        aSummary.mRobotVictories += aStats.mVictories;
        aSummary.mRobotDeaths += aStats.mDeaths;
        aSummary.mDolphinDeaths += aStats.mDolphinDeaths;
        aSummary.mDolphinCheeseSteals += aStats.mDolphinCheeseTriages;
        aSummary.mStarBursts += aStats.mStarBurst;
        aSummary.mChaosStorms += aStats.mChaosStorm;
        aSummary.mCometTrails += aStats.mCometTrailsHorizontal + aStats.mCometTrailsVertical;
        aCompletedMilliseconds += timing::ActiveTimingProfile().mMazeBlockMilliseconds;
      }
    }
    LogMazeSummaries(pRequest.mLogger, aMazeSummaries);

    if (pRequest.mLogger != nullptr) {
      ReportProgress(*pRequest.mLogger,
                     kDefaultModeName,
                     ProgressProfileKind::kBundle,
                     ProgressPhase::kExpansion,
                     aCompletedMilliseconds / aWork.mTotalMilliseconds,
                     "Maze simulation stage complete.");
    }
  }

  helpers::LogStatus(pRequest.mLogger, "Finalizing tables...");
  ResetRandomCursor();
  if (pRequest.mLogger != nullptr) {
    aCompletedMilliseconds += aWork.mFinalizeMilliseconds;
    ReportProgress(*pRequest.mLogger,
                   kDefaultModeName,
                   ProgressProfileKind::kBundle,
                   ProgressPhase::kFinalize,
                   aCompletedMilliseconds / aWork.mTotalMilliseconds,
                   "Finalizing tables.");
  }
  helpers::LogStatus(pRequest.mLogger, "Password expansion and table generation has completed.");
  if (pRequest.mLogger != nullptr) {
    ReportProgress(*pRequest.mLogger,
                   kDefaultModeName,
                   ProgressProfileKind::kBundle,
                   ProgressPhase::kFinalize,
                   1.0,
                   "Tables are ready.");
  }
  return true;
}

}  // namespace peanutbutter::tables
