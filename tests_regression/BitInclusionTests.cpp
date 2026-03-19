#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr int kDefaultTrialCount = 100;
constexpr int kDefaultBlockCount = 200;
constexpr std::size_t kWordListCount = 25U;

constexpr const char* kFruitWords[kWordListCount] = {
    "apple",      "apricot",   "banana",    "blackberry", "blueberry",
    "cherry",     "coconut",   "fig",       "grape",      "grapefruit",
    "guava",      "kiwi",      "lemon",     "lime",       "mango",
    "melon",      "nectarine", "orange",    "papaya",     "peach",
    "pear",       "pineapple", "plum",      "raspberry",  "strawberry",
};

constexpr const char* kAnimalWords[kWordListCount] = {
    "ant",       "badger",   "bear",      "beaver",   "bison",
    "cat",       "cougar",   "deer",      "dog",      "dolphin",
    "eagle",     "ferret",   "fox",       "frog",     "goat",
    "hawk",      "horse",    "koala",     "lemur",    "lion",
    "otter",     "panda",    "rabbit",    "tiger",    "wolf",
};

constexpr const char* kStyleWords[kWordListCount] = {
    "blazer",    "boots",     "cap",       "cardigan", "coat",
    "denim",     "dress",     "hoodie",    "jacket",   "jeans",
    "khakis",    "loafer",    "mittens",   "parka",    "poncho",
    "scarf",     "shirt",     "shorts",    "skirt",    "slippers",
    "sneakers",  "suit",      "sweater",   "tunic",    "vest",
};

const char* ExpanderTypeName(PasswordExpander::Type pType) {
  switch (pType) {
    case PasswordExpander::kType00:
      return "ExpandPassword_00";
    case PasswordExpander::kType01:
      return "ExpandPassword_01";
    case PasswordExpander::kType02:
      return "ExpandPassword_02";
    case PasswordExpander::kType03:
      return "ExpandPassword_03";
    case PasswordExpander::kType04:
      return "ExpandPassword_04";
    case PasswordExpander::kType05:
      return "ExpandPassword_05";
    case PasswordExpander::kType06:
      return "ExpandPassword_06";
    case PasswordExpander::kType07:
      return "ExpandPassword_07";
    case PasswordExpander::kType08:
      return "ExpandPassword_08";
    case PasswordExpander::kType09:
      return "ExpandPassword_09";
    case PasswordExpander::kType10:
      return "ExpandPassword_10";
    case PasswordExpander::kType11:
      return "ExpandPassword_11";
    case PasswordExpander::kType12:
      return "ExpandPassword_12";
    case PasswordExpander::kType13:
      return "ExpandPassword_13";
    case PasswordExpander::kType14:
      return "ExpandPassword_14";
    case PasswordExpander::kType15:
      return "ExpandPassword_15";
    case PasswordExpander::kTypeCount:
      break;
  }
  return "ExpandPassword_Unknown";
}

bool ParsePositiveInt(const char* pText, int* pOutValue) {
  if (pText == nullptr || pOutValue == nullptr) {
    return false;
  }

  int aValue = 0;
  for (const char* aCursor = pText; *aCursor != '\0'; ++aCursor) {
    if (*aCursor < '0' || *aCursor > '9') {
      return false;
    }
    aValue = (aValue * 10) + (*aCursor - '0');
    if (aValue <= 0) {
      return false;
    }
  }

  *pOutValue = aValue;
  return aValue > 0;
}

std::string BuildPassword(int pTrialIndex) {
  const std::size_t aIndex = static_cast<std::size_t>(pTrialIndex) % kWordListCount;
  const std::size_t aAnimalIndex = (aIndex * 7U + 3U) % kWordListCount;
  const std::size_t aStyleIndex = (aIndex * 11U + 5U) % kWordListCount;

  switch (pTrialIndex % 8) {
    case 0:
      return std::string(1, static_cast<char>('a' + static_cast<int>(aIndex % 26U)));
    case 1:
      return std::string() +
             static_cast<char>('a' + static_cast<int>(aIndex % 26U)) +
             static_cast<char>('a' + static_cast<int>(aAnimalIndex % 26U));
    case 2:
      return kFruitWords[aIndex];
    case 3:
      return kAnimalWords[aAnimalIndex];
    case 4:
      return kStyleWords[aStyleIndex];
    case 5:
      return std::string(kFruitWords[aIndex]) + " " + kAnimalWords[aAnimalIndex];
    case 6:
      return std::string(kAnimalWords[aAnimalIndex]) + " " + kStyleWords[aStyleIndex];
    default:
      return std::string(kFruitWords[aIndex]) + " " + kAnimalWords[aAnimalIndex] + " " + kStyleWords[aStyleIndex];
  }
}

std::uint64_t HashBytes(std::uint64_t pDigest, const unsigned char* pBytes, std::size_t pLength) {
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    pDigest = (pDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBytes[aIndex]);
  }
  return pDigest;
}

std::uint64_t CountChangedBitsAndMarkCoverage(const std::vector<unsigned char>& pBase,
                                              const std::vector<unsigned char>& pMutated,
                                              std::vector<unsigned char>* pCoverageBits) {
  std::uint64_t aChangedBits = 0U;
  for (std::size_t aByteIndex = 0; aByteIndex < pBase.size(); ++aByteIndex) {
    const unsigned char aDelta = static_cast<unsigned char>(pBase[aByteIndex] ^ pMutated[aByteIndex]);
    if (aDelta == 0U) {
      continue;
    }

    for (int aBitIndex = 0; aBitIndex < 8; ++aBitIndex) {
      const unsigned char aMask = static_cast<unsigned char>(1U << static_cast<unsigned int>(aBitIndex));
      if ((aDelta & aMask) == 0U) {
        continue;
      }
      ++aChangedBits;
      const std::size_t aGlobalBitIndex = (aByteIndex * 8U) + static_cast<std::size_t>(aBitIndex);
      (*pCoverageBits)[aGlobalBitIndex] = 1U;
    }
  }
  return aChangedBits;
}

std::uint64_t CountIncludedBits(const std::vector<unsigned char>& pCoverageBits) {
  std::uint64_t aIncludedBits = 0U;
  for (unsigned char aBit : pCoverageBits) {
    if (aBit != 0U) {
      ++aIncludedBits;
    }
  }
  return aIncludedBits;
}

struct AggregateStats {
  std::uint64_t mTrialCount = 0U;
  std::uint64_t mIncludedBits = 0U;
  std::uint64_t mUnincludedBits = 0U;
  long double mInclusionPercent = 0.0L;
  long double mAverageTrialPercent = 0.0L;
  long double mMinTrialPercent = 100.0L;
  long double mMaxTrialPercent = 0.0L;
  std::uint64_t mDigest = 1469598103934665603ULL;
};

template <typename TCounter>
void FillCounterOutput(const std::vector<unsigned char>& pSeed,
                       std::vector<unsigned char>* pOutput) {
  TCounter aCounter;
  aCounter.Seed(const_cast<unsigned char*>(pSeed.data()), static_cast<int>(pSeed.size()));
  aCounter.Get(pOutput->data(), static_cast<int>(pOutput->size()));
}

void FillExpanderOutput(PasswordExpander::Type pType,
                        const std::vector<unsigned char>& pSeed,
                        std::array<unsigned char, PASSWORD_EXPANDED_SIZE>* pWorker,
                        std::vector<unsigned char>* pOutput) {
  PasswordExpander::ExpandPasswordBlocks(
      pType,
      const_cast<unsigned char*>(pSeed.data()),
      static_cast<unsigned int>(pSeed.size()),
      pWorker->data(),
      pOutput->data(),
      static_cast<unsigned int>(pOutput->size()));
}

template <typename TFillFn>
AggregateStats RunBitInclusionTrials(TFillFn pFillFn,
                                     int pTrialCount,
                                     std::uint64_t pOutputBitLength) {
  const std::size_t aOutputLength = static_cast<std::size_t>(pOutputBitLength / 8ULL);
  std::vector<unsigned char> aCoverageBits(static_cast<std::size_t>(pOutputBitLength), 0U);
  std::vector<unsigned char> aBaseOutput(aOutputLength, 0U);
  std::vector<unsigned char> aMutatedOutput(aOutputLength, 0U);
  AggregateStats aStats;
  long double aTotalTrialPercent = 0.0L;

  for (int aTrialIndex = 0; aTrialIndex < pTrialCount; ++aTrialIndex) {
    const std::string aPassword = BuildPassword(aTrialIndex);
    std::vector<unsigned char> aBaseSeed(aPassword.begin(), aPassword.end());
    std::vector<unsigned char> aMutatedSeed = aBaseSeed;

    if (aMutatedSeed.empty()) {
      aMutatedSeed.push_back(0U);
    }

    const std::size_t aByteIndex =
        static_cast<std::size_t>(aTrialIndex) % static_cast<std::size_t>(aMutatedSeed.size());
    const unsigned char aMask =
        static_cast<unsigned char>(1U << static_cast<unsigned int>((aTrialIndex / 3) % 8));
    aMutatedSeed[aByteIndex] ^= aMask;

    pFillFn(aBaseSeed, &aBaseOutput, false);
    pFillFn(aMutatedSeed, &aMutatedOutput, true);

    const std::uint64_t aChangedBits =
        CountChangedBitsAndMarkCoverage(aBaseOutput, aMutatedOutput, &aCoverageBits);
    const long double aTrialPercent =
        (pOutputBitLength == 0U)
            ? 0.0L
            : (static_cast<long double>(aChangedBits) * 100.0L) / static_cast<long double>(pOutputBitLength);

    ++aStats.mTrialCount;
    aTotalTrialPercent += aTrialPercent;
    if (aTrialPercent < aStats.mMinTrialPercent) {
      aStats.mMinTrialPercent = aTrialPercent;
    }
    if (aTrialPercent > aStats.mMaxTrialPercent) {
      aStats.mMaxTrialPercent = aTrialPercent;
    }
    aStats.mDigest = HashBytes(aStats.mDigest, aBaseOutput.data(), aBaseOutput.size());
    aStats.mDigest = HashBytes(aStats.mDigest, aMutatedOutput.data(), aMutatedOutput.size());
  }

  aStats.mIncludedBits = CountIncludedBits(aCoverageBits);
  aStats.mUnincludedBits = pOutputBitLength - aStats.mIncludedBits;
  aStats.mInclusionPercent =
      (pOutputBitLength == 0U)
          ? 0.0L
          : (static_cast<long double>(aStats.mIncludedBits) * 100.0L) / static_cast<long double>(pOutputBitLength);
  aStats.mAverageTrialPercent =
      (aStats.mTrialCount == 0U) ? 0.0L : aTotalTrialPercent / static_cast<long double>(aStats.mTrialCount);
  return aStats;
}

void PrintStatsLine(const char* pName,
                    const AggregateStats& pStats,
                    std::uint64_t pOutputBitLength) {
  std::cout << std::left << std::setw(20) << pName
            << " trials=" << pStats.mTrialCount
            << " inclusion_percent=" << std::fixed << std::setprecision(4)
            << static_cast<double>(pStats.mInclusionPercent)
            << " included_bits=" << pStats.mIncludedBits
            << " unincluded_bits=" << pStats.mUnincludedBits
            << " avg_trial_percent=" << static_cast<double>(pStats.mAverageTrialPercent)
            << " min_trial_percent=" << static_cast<double>(pStats.mMinTrialPercent)
            << " max_trial_percent=" << static_cast<double>(pStats.mMaxTrialPercent)
            << " total_bits=" << pOutputBitLength
            << " digest=" << pStats.mDigest
            << "\n";
}

}  // namespace

int main(int pArgc, char** pArgv) {
  int aTrialCount = kDefaultTrialCount;
  int aBlockCount = kDefaultBlockCount;
  if (pArgc >= 2 && !ParsePositiveInt(pArgv[1], &aTrialCount)) {
    std::cerr << "[FAIL] invalid trial count\n";
    return 1;
  }
  if (pArgc >= 3 && !ParsePositiveInt(pArgv[2], &aBlockCount)) {
    std::cerr << "[FAIL] invalid block count\n";
    return 1;
  }

  const std::size_t aOutputLength =
      static_cast<std::size_t>(aBlockCount) * static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE);
  const std::uint64_t aOutputBitLength = static_cast<std::uint64_t>(aOutputLength) * 8ULL;

  std::cout << "[INFO] bit inclusion tests"
            << " trials=" << aTrialCount
            << " blocks=" << aBlockCount
            << " block_bytes=" << PASSWORD_EXPANDED_SIZE
            << " output_bytes=" << aOutputLength
            << " output_bits=" << aOutputBitLength
            << "\n";
  std::cout << "[INFO] one input bit is flipped per trial; inclusion means an output bit changed at least once across sampled trials\n";

  for (int aTypeIndex = 0; aTypeIndex < PasswordExpander::kTypeCount; ++aTypeIndex) {
    const PasswordExpander::Type aType = static_cast<PasswordExpander::Type>(aTypeIndex);
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorkerBase = {};
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorkerMutated = {};
    const AggregateStats aStats = RunBitInclusionTrials(
        [&](const std::vector<unsigned char>& pSeed,
            std::vector<unsigned char>* pOutput,
            bool pMutated) {
          if (pMutated) {
            FillExpanderOutput(aType, pSeed, &aWorkerMutated, pOutput);
          } else {
            FillExpanderOutput(aType, pSeed, &aWorkerBase, pOutput);
          }
        },
        aTrialCount,
        aOutputBitLength);
    PrintStatsLine(ExpanderTypeName(aType), aStats, aOutputBitLength);
  }

  const AggregateStats aAesStats = RunBitInclusionTrials(
      [](const std::vector<unsigned char>& pSeed,
         std::vector<unsigned char>* pOutput,
         bool) {
        FillCounterOutput<AESCounter>(pSeed, pOutput);
      },
      aTrialCount,
      aOutputBitLength);
  PrintStatsLine("AESCounter", aAesStats, aOutputBitLength);

  const AggregateStats aAriaStats = RunBitInclusionTrials(
      [](const std::vector<unsigned char>& pSeed,
         std::vector<unsigned char>* pOutput,
         bool) {
        FillCounterOutput<ARIA256Counter>(pSeed, pOutput);
      },
      aTrialCount,
      aOutputBitLength);
  PrintStatsLine("ARIA256Counter", aAriaStats, aOutputBitLength);

  const AggregateStats aChaChaStats = RunBitInclusionTrials(
      [](const std::vector<unsigned char>& pSeed,
         std::vector<unsigned char>* pOutput,
         bool) {
        FillCounterOutput<ChaCha20Counter>(pSeed, pOutput);
      },
      aTrialCount,
      aOutputBitLength);
  PrintStatsLine("ChaCha20Counter", aChaChaStats, aOutputBitLength);

  std::cout << "[PASS] bit inclusion tests completed\n";
  return 0;
}
