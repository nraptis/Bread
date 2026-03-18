#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/Tables.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/Tests.hpp"

#ifndef PB_STAGE_BENCH_MODE
#define PB_STAGE_BENCH_MODE "unknown"
#endif

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;
using peanutbutter::games::GameBoard;
using peanutbutter::maze::MazeDirector;

struct FillInstruction {
  enum Kind {
    kAes = 0,
    kAria = 1,
    kChaCha = 2,
    kExpander = 3,
  };

  Kind mKind = kAes;
  unsigned char mExpanderIndex = 0U;
};

struct TablePlan {
  const char* mName = nullptr;
  std::size_t mSize = 0U;
  FillInstruction mInstruction{};
};

struct StageStat {
  std::string mName;
  double mTotalMilliseconds = 0.0;
  double mMinMilliseconds = 0.0;
  double mMaxMilliseconds = 0.0;
  int mCount = 0;

  void Add(double pMilliseconds) {
    if (mCount == 0) {
      mMinMilliseconds = pMilliseconds;
      mMaxMilliseconds = pMilliseconds;
    } else {
      if (pMilliseconds < mMinMilliseconds) {
        mMinMilliseconds = pMilliseconds;
      }
      if (pMilliseconds > mMaxMilliseconds) {
        mMaxMilliseconds = pMilliseconds;
      }
    }
    mTotalMilliseconds += pMilliseconds;
    ++mCount;
  }
};

constexpr int kIterations = 10;
constexpr std::size_t kScratchSize = peanutbutter::tables::Tables::kTableCount;

std::uint64_t HashBytes(const unsigned char* pBytes, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBytes[aIndex]);
  }
  return aDigest;
}

std::uint64_t HashValue(std::uint64_t pDigest, std::uint64_t pValue) {
  for (int aShift = 0; aShift < 64; aShift += 8) {
    pDigest = (pDigest * 1099511628211ULL) ^ ((pValue >> aShift) & 0xFFU);
  }
  return pDigest;
}

double MillisecondsSince(const std::chrono::steady_clock::time_point& pStartedAt,
                         const std::chrono::steady_clock::time_point& pEndedAt) {
  return std::chrono::duration<double, std::milli>(pEndedAt - pStartedAt).count();
}

std::array<unsigned char, kScratchSize> BuildScratch(const std::string& pPassword) {
  std::array<unsigned char, kScratchSize> aScratch = {};
  if (pPassword.empty()) {
    return aScratch;
  }
  for (std::size_t aIndex = 0; aIndex < aScratch.size(); ++aIndex) {
    aScratch[aIndex] = static_cast<unsigned char>(pPassword[aIndex % pPassword.size()]);
  }
  return aScratch;
}

template <std::size_t kCount>
void ShuffleInstructions(std::array<FillInstruction, kCount>& pInstructions,
                         const std::array<unsigned char, kScratchSize>& pScratch,
                         std::size_t pScratchOffset) {
  for (std::size_t aIndex = 0; aIndex < kCount; ++aIndex) {
    const std::size_t aRemaining = kCount - aIndex;
    const std::size_t aSwapIndex =
        aIndex + (static_cast<std::size_t>(pScratch[(pScratchOffset + aIndex) % pScratch.size()]) % aRemaining);
    std::swap(pInstructions[aIndex], pInstructions[aSwapIndex]);
  }
}

std::array<unsigned char, PasswordExpander::kTypeCount> BuildExpanderStack(
    const std::array<unsigned char, kScratchSize>& pScratch) {
  std::array<unsigned char, PasswordExpander::kTypeCount> aStack = {};
  for (int aIndex = 0; aIndex < PasswordExpander::kTypeCount; ++aIndex) {
    aStack[static_cast<std::size_t>(aIndex)] = static_cast<unsigned char>(aIndex);
  }
  for (std::size_t aIndex = 0; aIndex < aStack.size(); ++aIndex) {
    const std::size_t aRemaining = aStack.size() - aIndex;
    const std::size_t aSwapIndex =
        aIndex + (static_cast<std::size_t>(pScratch[(3U + aIndex) % pScratch.size()]) % aRemaining);
    std::swap(aStack[aIndex], aStack[aSwapIndex]);
  }
  return aStack;
}

FillInstruction NextExpanderInstruction(const std::array<unsigned char, PasswordExpander::kTypeCount>& pStack,
                                        std::size_t* pIndex) {
  const std::size_t aStackIndex = (*pIndex) % pStack.size();
  ++(*pIndex);
  FillInstruction aInstruction;
  aInstruction.mKind = FillInstruction::kExpander;
  aInstruction.mExpanderIndex = pStack[aStackIndex];
  return aInstruction;
}

std::vector<TablePlan> BuildTablePlan(const std::string& pPassword) {
  const auto aScratch = BuildScratch(pPassword);
  const auto aExpanderStack = BuildExpanderStack(aScratch);
  std::size_t aExpanderIndex = 0U;
  const bool aUseAriaOnL1 = (aScratch[0] & 1U) == 0U;

  std::array<FillInstruction, peanutbutter::tables::Tables::kL1TableCount> aL1 = {{
      {FillInstruction::kAes, 0U},
      {aUseAriaOnL1 ? FillInstruction::kAria : FillInstruction::kChaCha, 0U},
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
  }};
  std::array<FillInstruction, peanutbutter::tables::Tables::kL2TableCount> aL2 = {{
      {FillInstruction::kAes, 0U},
      {aUseAriaOnL1 ? FillInstruction::kChaCha : FillInstruction::kAria, 0U},
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
  }};
  std::array<FillInstruction, peanutbutter::tables::Tables::kL3TableCount> aL3 = {{
      {FillInstruction::kAes, 0U},
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
      NextExpanderInstruction(aExpanderStack, &aExpanderIndex),
  }};

  ShuffleInstructions(aL1, aScratch, 1U);
  ShuffleInstructions(aL2, aScratch, 7U);
  ShuffleInstructions(aL3, aScratch, 13U);

  const char* kL1Names[] = {"L1_A","L1_B","L1_C","L1_D","L1_E","L1_F","L1_G","L1_H","L1_I","L1_J","L1_K","L1_L"};
  const char* kL2Names[] = {"L2_A","L2_B","L2_C","L2_D","L2_E","L2_F"};
  const char* kL3Names[] = {"L3_A","L3_B","L3_C","L3_D"};

  std::vector<TablePlan> aPlan;
  for (std::size_t aIndex = 0; aIndex < aL1.size(); ++aIndex) {
    aPlan.push_back({kL1Names[aIndex], BLOCK_SIZE_L1, aL1[aIndex]});
  }
  for (std::size_t aIndex = 0; aIndex < aL2.size(); ++aIndex) {
    aPlan.push_back({kL2Names[aIndex], BLOCK_SIZE_L2, aL2[aIndex]});
  }
  for (std::size_t aIndex = 0; aIndex < aL3.size(); ++aIndex) {
    aPlan.push_back({kL3Names[aIndex], BLOCK_SIZE_L3, aL3[aIndex]});
  }
  return aPlan;
}

void SeedAes(const std::string& pPassword, unsigned char* pDestination, std::size_t pLength) {
  AESCounter aCounter;
  aCounter.Seed(reinterpret_cast<unsigned char*>(const_cast<char*>(pPassword.data())), static_cast<int>(pPassword.size()));
  aCounter.Get(pDestination, static_cast<int>(pLength));
}

void SeedAria(const std::string& pPassword, unsigned char* pDestination, std::size_t pLength) {
  ARIA256Counter aCounter;
  aCounter.Seed(reinterpret_cast<unsigned char*>(const_cast<char*>(pPassword.data())), static_cast<int>(pPassword.size()));
  aCounter.Get(pDestination, static_cast<int>(pLength));
}

void SeedChaCha(const std::string& pPassword, unsigned char* pDestination, std::size_t pLength) {
  ChaCha20Counter aCounter;
  aCounter.Seed(reinterpret_cast<unsigned char*>(const_cast<char*>(pPassword.data())), static_cast<int>(pPassword.size()));
  aCounter.Get(pDestination, static_cast<int>(pLength));
}

void SeedExpander(const std::string& pPassword,
                  unsigned char pExpanderIndex,
                  unsigned char* pDestination,
                  std::size_t pLength) {
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  PasswordExpander::ExpandPasswordBlocksByIndex(pExpanderIndex,
                                                reinterpret_cast<unsigned char*>(const_cast<char*>(pPassword.data())),
                                                static_cast<unsigned int>(pPassword.size()),
                                                aWorker.data(),
                                                pDestination,
                                                static_cast<unsigned int>(pLength));
}

std::array<int, 16> BuildPlayOrder(const unsigned char* pL1A) {
  std::array<int, 16> aOrder = {};
  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    aOrder[aIndex] = aIndex;
  }
  for (int aIndex = 0; aIndex < 16; ++aIndex) {
    const int aWhich = static_cast<int>(pL1A[aIndex] % 16U);
    const int aTemp = aOrder[aIndex];
    aOrder[aIndex] = aOrder[aWhich];
    aOrder[aWhich] = aTemp;
  }
  return aOrder;
}

StageStat& FindOrCreateStage(std::vector<StageStat>* pStages, const std::string& pName) {
  for (StageStat& aStage : *pStages) {
    if (aStage.mName == pName) {
      return aStage;
    }
  }
  pStages->push_back(StageStat{pName});
  return pStages->back();
}

std::string FormatGameBlockLabel(std::size_t pBlockIndex) {
  char aBuffer[64];
  std::snprintf(aBuffer, sizeof(aBuffer), "game simulation block %03zu", pBlockIndex);
  return std::string(aBuffer);
}

std::string FormatMazeBlockLabel(std::size_t pBlockIndex) {
  char aBuffer[64];
  std::snprintf(aBuffer, sizeof(aBuffer), "maze simulation block %03zu", pBlockIndex);
  return std::string(aBuffer);
}

}  // namespace

int main(int pArgc, char** pArgv) {
  if (!peanutbutter::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const std::string aPassword = (pArgc >= 2 && pArgv[1] != nullptr) ? pArgv[1] : "hotdog";
  const peanutbutter::tables::GameStyle aGameStyle =
      (pArgc >= 3 && std::string(pArgv[2]) == "full") ? peanutbutter::tables::GameStyle::kFull
                                                       : peanutbutter::tables::GameStyle::kSparse;
  const peanutbutter::tables::MazeStyle aMazeStyle =
      (pArgc >= 4 && std::string(pArgv[3]) == "full") ? peanutbutter::tables::MazeStyle::kFull
                                                       : peanutbutter::tables::MazeStyle::kSparse;

  const std::vector<TablePlan> aPlan = BuildTablePlan(aPassword);
  std::vector<StageStat> aStages;
  std::uint64_t aDigest = 1469598103934665603ULL;

  for (int aIteration = 0; aIteration < kIterations; ++aIteration) {
    std::vector<std::vector<unsigned char>> aBuffers(aPlan.size());
    for (std::size_t aIndex = 0; aIndex < aPlan.size(); ++aIndex) {
      aBuffers[aIndex].assign(aPlan[aIndex].mSize, 0U);
      const auto aStartedAt = std::chrono::steady_clock::now();
      switch (aPlan[aIndex].mInstruction.mKind) {
        case FillInstruction::kAes:
          SeedAes(aPassword, aBuffers[aIndex].data(), aBuffers[aIndex].size());
          break;
        case FillInstruction::kAria:
          SeedAria(aPassword, aBuffers[aIndex].data(), aBuffers[aIndex].size());
          break;
        case FillInstruction::kChaCha:
          SeedChaCha(aPassword, aBuffers[aIndex].data(), aBuffers[aIndex].size());
          break;
        case FillInstruction::kExpander:
          SeedExpander(aPassword, aPlan[aIndex].mInstruction.mExpanderIndex, aBuffers[aIndex].data(), aBuffers[aIndex].size());
          break;
      }
      const auto aEndedAt = std::chrono::steady_clock::now();
      FindOrCreateStage(&aStages,
                        std::string("seeding buffer ") + aPlan[aIndex].mName)
          .Add(MillisecondsSince(aStartedAt, aEndedAt));
      aDigest = HashValue(aDigest, HashBytes(aBuffers[aIndex].data(), aBuffers[aIndex].size()));
    }

    const std::array<int, 16> aPlayOrder = BuildPlayOrder(aBuffers[0].data());

    if (aGameStyle != peanutbutter::tables::GameStyle::kNone) {
      const std::size_t aStride = (aGameStyle == peanutbutter::tables::GameStyle::kSparse) ? 4U : 1U;
      std::array<unsigned char, GameBoard::kSeedBufferCapacity> aOutput = {};
      GameBoard aBoard;
      std::size_t aGlobalBlockIndex = 0U;
      for (std::size_t aTableIndex = 0; aTableIndex < aPlan.size(); ++aTableIndex) {
        if ((aPlan[aTableIndex].mSize % static_cast<std::size_t>(GameBoard::kSeedBufferCapacity)) != 0U) {
          continue;
        }
        const std::size_t aBlockCount = aPlan[aTableIndex].mSize / static_cast<std::size_t>(GameBoard::kSeedBufferCapacity);
        std::size_t aProcessedIndex = 0U;
        for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
          unsigned char* const aBlock =
              aBuffers[aTableIndex].data() + (aBlockIndex * static_cast<std::size_t>(GameBoard::kSeedBufferCapacity));
          const auto aStartedAt = std::chrono::steady_clock::now();
          aBoard.SetGame(aPlayOrder[aProcessedIndex % aPlayOrder.size()]);
          aBoard.Seed(aBlock, GameBoard::kSeedBufferCapacity);
          aBoard.Get(aOutput.data(), static_cast<int>(aOutput.size()));
          std::memcpy(aBlock, aOutput.data(), aOutput.size());
          const auto aEndedAt = std::chrono::steady_clock::now();
          FindOrCreateStage(&aStages, FormatGameBlockLabel(aGlobalBlockIndex))
              .Add(MillisecondsSince(aStartedAt, aEndedAt));
          aDigest = HashValue(aDigest, HashBytes(aOutput.data(), aOutput.size()));
          ++aProcessedIndex;
          ++aGlobalBlockIndex;
        }
      }
    }

    if (aMazeStyle != peanutbutter::tables::MazeStyle::kNone) {
      const std::size_t aStride = (aMazeStyle == peanutbutter::tables::MazeStyle::kSparse) ? 4U : 1U;
      std::array<unsigned char, MazeDirector::kSeedBufferCapacity> aOutput = {};
      MazeDirector aMaze;
      std::size_t aGlobalBlockIndex = 0U;
      for (std::size_t aTableIndex = 0; aTableIndex < aPlan.size(); ++aTableIndex) {
        if ((aPlan[aTableIndex].mSize % static_cast<std::size_t>(MazeDirector::kSeedBufferCapacity)) != 0U) {
          continue;
        }
        const std::size_t aBlockCount = aPlan[aTableIndex].mSize / static_cast<std::size_t>(MazeDirector::kSeedBufferCapacity);
        std::size_t aProcessedIndex = 0U;
        for (std::size_t aBlockIndex = 0; aBlockIndex < aBlockCount; aBlockIndex += aStride) {
          unsigned char* const aBlock =
              aBuffers[aTableIndex].data() + (aBlockIndex * static_cast<std::size_t>(MazeDirector::kSeedBufferCapacity));
          const auto aStartedAt = std::chrono::steady_clock::now();
          aMaze.SetGame(aPlayOrder[aProcessedIndex % aPlayOrder.size()]);
          aMaze.Seed(aBlock, MazeDirector::kSeedBufferCapacity);
          aMaze.Get(aOutput.data(), static_cast<int>(aOutput.size()));
          std::memcpy(aBlock, aOutput.data(), aOutput.size());
          const auto aEndedAt = std::chrono::steady_clock::now();
          FindOrCreateStage(&aStages, FormatMazeBlockLabel(aGlobalBlockIndex))
              .Add(MillisecondsSince(aStartedAt, aEndedAt));
          aDigest = HashValue(aDigest, HashBytes(aOutput.data(), aOutput.size()));
          ++aProcessedIndex;
          ++aGlobalBlockIndex;
        }
      }
    }
  }

  std::cout << "[INFO] tables stage benchmark"
            << " mode=" << PB_STAGE_BENCH_MODE
            << " iterations=" << kIterations
            << " password=" << aPassword
            << " game_style=" << peanutbutter::tables::GameStyleName(aGameStyle)
            << " maze_style=" << peanutbutter::tables::MazeStyleName(aMazeStyle)
            << "\n";

  for (const StageStat& aStage : aStages) {
    const double aAverage = (aStage.mCount > 0) ? (aStage.mTotalMilliseconds / static_cast<double>(aStage.mCount)) : 0.0;
    std::cout << "[STAGE] name=" << aStage.mName
              << " avg_ms=" << std::fixed << std::setprecision(3) << aAverage
              << " min_ms=" << aStage.mMinMilliseconds
              << " max_ms=" << aStage.mMaxMilliseconds
              << " samples=" << aStage.mCount << "\n";
  }

  std::cout << "[PASS] tables stage benchmark complete"
            << " mode=" << PB_STAGE_BENCH_MODE
            << " digest=" << aDigest << "\n";
  return 0;
}
