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
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"
#include "tests/common/Tests.hpp"

namespace {

using peanutbutter::expansion::key_expansion::ByteTwister;
using peanutbutter::expansion::key_expansion::PasswordExpander;

struct TableDescriptor {
  const char* mName;
  unsigned char* mData;
  std::size_t mSize;
};

struct RunMetrics {
  std::uint64_t mTableDigest = 0U;
  std::uint64_t mEngineDigest = 0U;
  double mFillMilliseconds = 0.0;
  double mEngineMilliseconds = 0.0;
  std::size_t mFilledBytes = 0U;
  std::size_t mEngineBlocks = 0U;
};

constexpr TableDescriptor kTables[] = {
    {"L1_A", gTableL1_A, BLOCK_SIZE_L1}, {"L1_B", gTableL1_B, BLOCK_SIZE_L1}, {"L1_C", gTableL1_C, BLOCK_SIZE_L1},
    {"L1_D", gTableL1_D, BLOCK_SIZE_L1}, {"L1_E", gTableL1_E, BLOCK_SIZE_L1}, {"L1_F", gTableL1_F, BLOCK_SIZE_L1},
    {"L1_G", gTableL1_G, BLOCK_SIZE_L1}, {"L1_H", gTableL1_H, BLOCK_SIZE_L1}, {"L2_A", gTableL2_A, BLOCK_SIZE_L2},
    {"L2_B", gTableL2_B, BLOCK_SIZE_L2}, {"L2_C", gTableL2_C, BLOCK_SIZE_L2}, {"L2_D", gTableL2_D, BLOCK_SIZE_L2},
    {"L2_E", gTableL2_E, BLOCK_SIZE_L2}, {"L2_F", gTableL2_F, BLOCK_SIZE_L2}, {"L3_A", gTableL3_A, BLOCK_SIZE_L3},
    {"L3_B", gTableL3_B, BLOCK_SIZE_L3}, {"L3_C", gTableL3_C, BLOCK_SIZE_L3}, {"L3_D", gTableL3_D, BLOCK_SIZE_L3},
};

constexpr std::size_t kSeedMaterialLength = 64U;

std::uint32_t NextState(std::uint32_t pState) {
  std::uint32_t aState = pState;
  aState ^= (aState << 13U);
  aState ^= (aState >> 17U);
  aState ^= (aState << 5U);
  return aState;
}

void FillPattern(unsigned char* pBuffer, std::size_t pLength, std::uint32_t pSeed) {
  std::uint32_t aState = pSeed;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aState = NextState(aState + static_cast<std::uint32_t>(aIndex + 1U));
    pBuffer[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

std::uint64_t HashBytes(const unsigned char* pBuffer, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBuffer[aIndex]);
  }
  return aDigest;
}

std::uint64_t HashValue(std::uint64_t pDigest, std::uint64_t pValue) {
  for (int aShift = 0; aShift < 64; aShift += 8) {
    pDigest = (pDigest * 1099511628211ULL) ^ ((pValue >> aShift) & 0xFFU);
  }
  return pDigest;
}

std::uint32_t SeedFromText(const std::string& pSeedText, unsigned char pType, const char* pLabel) {
  std::uint32_t aState = 0x811C9DC5U ^ static_cast<std::uint32_t>(pType + 1U);
  for (char aChar : pSeedText) {
    aState ^= static_cast<unsigned char>(aChar);
    aState *= 16777619U;
  }
  for (const char* aIt = pLabel; aIt != nullptr && *aIt != '\0'; ++aIt) {
    aState ^= static_cast<unsigned char>(*aIt);
    aState *= 16777619U;
  }
  return aState;
}

std::array<unsigned char, kSeedMaterialLength> BuildSeedMaterial(const std::string& pSeedText,
                                                                 unsigned char pType,
                                                                 const char* pLabel) {
  std::array<unsigned char, kSeedMaterialLength> aSeed = {};
  std::uint32_t aState = SeedFromText(pSeedText, pType, pLabel);
  for (std::size_t aIndex = 0; aIndex < aSeed.size(); ++aIndex) {
    aState = NextState(aState + static_cast<std::uint32_t>((aIndex * 33U) + 17U));
    aSeed[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
  return aSeed;
}

void PoisonTables(std::uint32_t pSeedBase) {
  for (std::size_t aTableIndex = 0; aTableIndex < (sizeof(kTables) / sizeof(kTables[0])); ++aTableIndex) {
    FillPattern(kTables[aTableIndex].mData,
                kTables[aTableIndex].mSize,
                pSeedBase + static_cast<std::uint32_t>((aTableIndex + 1U) * 0x1021U));
  }
}

bool RunScenario(const std::string& pSeedText,
                 unsigned char pType,
                 std::uint32_t pPoisonBase,
                 std::size_t pEngineBlockLimit,
                 RunMetrics* pOutMetrics) {
  if (pOutMetrics == nullptr) {
    return false;
  }

  *pOutMetrics = RunMetrics{};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};

  PoisonTables(pPoisonBase);
  auto aFillStart = std::chrono::steady_clock::now();
  for (const TableDescriptor& aTable : kTables) {
    const std::array<unsigned char, kSeedMaterialLength> aSeed = BuildSeedMaterial(pSeedText, pType, aTable.mName);
    FillPattern(aWorker.data(),
                aWorker.size(),
                pPoisonBase ^ static_cast<std::uint32_t>(aTable.mSize) ^ SeedFromText(pSeedText, pType, aTable.mName));
    PasswordExpander::ExpandPasswordBlocksByIndex(pType,
                                                  const_cast<unsigned char*>(aSeed.data()),
                                                  static_cast<unsigned int>(aSeed.size()),
                                                  aWorker.data(),
                                                  aTable.mData,
                                                  static_cast<unsigned int>(aTable.mSize));
    pOutMetrics->mFilledBytes += aTable.mSize;
    pOutMetrics->mTableDigest = HashValue(pOutMetrics->mTableDigest, HashBytes(aTable.mData, aTable.mSize));
  }
  auto aFillEnd = std::chrono::steady_clock::now();
  pOutMetrics->mFillMilliseconds =
      std::chrono::duration<double, std::milli>(aFillEnd - aFillStart).count();

  auto aEngineStart = std::chrono::steady_clock::now();
  std::vector<unsigned char> aOutput(static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE), 0U);
  std::size_t aBlockIndex = 0U;
  for (const TableDescriptor& aTable : kTables) {
    for (std::size_t aOffset = 0U; aOffset < aTable.mSize; aOffset += static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE)) {
      if (pEngineBlockLimit != 0U && aBlockIndex >= pEngineBlockLimit) {
        break;
      }

      peanutbutter::games::GameBoard aBoard;
      aBoard.SetGame(static_cast<int>(aBlockIndex % static_cast<std::size_t>(peanutbutter::games::GameBoard::kGameCount)));
      aBoard.Seed(aTable.mData + aOffset, PASSWORD_EXPANDED_SIZE);
      aBoard.Get(aOutput.data(), static_cast<int>(aOutput.size()));

      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, HashBytes(aOutput.data(), aOutput.size()));
      const peanutbutter::games::GameBoard::RuntimeStats aStats = aBoard.GetRuntimeStats();
      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, aStats.mStuck);
      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, aStats.mTopple);
      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, aStats.mUserMatch);
      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, aStats.mCascadeMatch);
      pOutMetrics->mEngineDigest = HashValue(pOutMetrics->mEngineDigest, aStats.mPasswordExpanderWraps);
      ++pOutMetrics->mEngineBlocks;
      ++aBlockIndex;
    }
    if (pEngineBlockLimit != 0U && aBlockIndex >= pEngineBlockLimit) {
      break;
    }
  }
  auto aEngineEnd = std::chrono::steady_clock::now();
  pOutMetrics->mEngineMilliseconds =
      std::chrono::duration<double, std::milli>(aEngineEnd - aEngineStart).count();
  return true;
}

std::size_t ParseOptionalSize(const char* pText, std::size_t pDefaultValue) {
  if (pText == nullptr || *pText == '\0') {
    return pDefaultValue;
  }
  const unsigned long long aValue = std::strtoull(pText, nullptr, 10);
  return static_cast<std::size_t>(aValue);
}

}  // namespace

int main(int pArgc, char** pArgv) {
  if (!peanutbutter::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] benchmark disabled by Tests.hpp\n";
    return 0;
  }

  const std::string aSeedText = (pArgc >= 2 && pArgv[1] != nullptr) ? pArgv[1] : "hotdog";
  const std::size_t aTypeCount = ParseOptionalSize((pArgc >= 3) ? pArgv[2] : nullptr, ByteTwister::kTypeCount);
  const std::size_t aEngineBlockLimit = ParseOptionalSize((pArgc >= 4) ? pArgv[3] : nullptr, 0U);
  const std::size_t aClampedTypeCount =
      (aTypeCount > static_cast<std::size_t>(ByteTwister::kTypeCount)) ? static_cast<std::size_t>(ByteTwister::kTypeCount)
                                                                        : aTypeCount;

  std::uint64_t aDigest = 1469598103934665603ULL;
  std::cout << "[INFO] table fill and engine benchmark"
            << " seed=" << aSeedText
            << " type_count=" << aClampedTypeCount
            << " engine_block_limit=" << aEngineBlockLimit
            << " total_table_bytes=" << (8U * BLOCK_SIZE_L1) + (6U * BLOCK_SIZE_L2) + (4U * BLOCK_SIZE_L3)
            << "\n";

  for (std::size_t aTypeIndex = 0U; aTypeIndex < aClampedTypeCount; ++aTypeIndex) {
    RunMetrics aRunA;
    RunMetrics aRunB;
    const unsigned char aType = static_cast<unsigned char>(aTypeIndex);
    if (!RunScenario(aSeedText, aType, 0x13572468U + static_cast<std::uint32_t>(aTypeIndex * 17U), aEngineBlockLimit, &aRunA) ||
        !RunScenario(aSeedText, aType, 0x24681357U + static_cast<std::uint32_t>(aTypeIndex * 29U), aEngineBlockLimit, &aRunB)) {
      std::cerr << "[FAIL] benchmark scenario execution failed for type=" << aTypeIndex << "\n";
      return 1;
    }

    if (aRunA.mTableDigest != aRunB.mTableDigest || aRunA.mEngineDigest != aRunB.mEngineDigest) {
      std::cerr << "[FAIL] deterministic benchmark mismatch for type=" << aTypeIndex
                << " table_a=" << aRunA.mTableDigest
                << " table_b=" << aRunB.mTableDigest
                << " engine_a=" << aRunA.mEngineDigest
                << " engine_b=" << aRunB.mEngineDigest << "\n";
      return 1;
    }

    const double aFillMiBPerSecond =
        (aRunA.mFillMilliseconds > 0.0)
            ? ((static_cast<double>(aRunA.mFilledBytes) / (1024.0 * 1024.0)) / (aRunA.mFillMilliseconds / 1000.0))
            : 0.0;
    const double aEngineMiBPerSecond =
        (aRunA.mEngineMilliseconds > 0.0)
            ? (((static_cast<double>(aRunA.mEngineBlocks) * static_cast<double>(PASSWORD_EXPANDED_SIZE)) / (1024.0 * 1024.0)) /
               (aRunA.mEngineMilliseconds / 1000.0))
            : 0.0;

    std::cout << "[TYPE] type=" << aTypeIndex
              << " verify=PASS"
              << " fill_ms=" << std::fixed << std::setprecision(3) << aRunA.mFillMilliseconds
              << " fill_MiB_s=" << aFillMiBPerSecond
              << " engine_ms=" << aRunA.mEngineMilliseconds
              << " engine_MiB_s=" << aEngineMiBPerSecond
              << " engine_blocks=" << aRunA.mEngineBlocks
              << " table_digest=" << aRunA.mTableDigest
              << " engine_digest=" << aRunA.mEngineDigest << "\n";

    aDigest = HashValue(aDigest, aRunA.mTableDigest);
    aDigest = HashValue(aDigest, aRunA.mEngineDigest);
  }

  std::cout << "[PASS] table fill and engine benchmark complete"
            << " seed=" << aSeedText
            << " type_count=" << aClampedTypeCount
            << " digest=" << aDigest << "\n";
  return 0;
}
