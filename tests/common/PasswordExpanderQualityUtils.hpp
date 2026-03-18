#ifndef BREAD_TESTS_COMMON_PASSWORD_EXPANDER_QUALITY_UTILS_HPP_
#define BREAD_TESTS_COMMON_PASSWORD_EXPANDER_QUALITY_UTILS_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/counters/AESCounter.hpp"
#include "src/Tables/counters/ARIA256Counter.hpp"
#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace peanutbutter::tests::password_expander_quality {

using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr int kTwisterCount = PasswordExpander::kTypeCount;
constexpr int kCounterCount = 3;
constexpr int kWindow64Bytes = 64;
constexpr int kWindow16Bytes = 16;

inline constexpr std::array<const char*, kTwisterCount> kTwisterLabels = {
    "BT00", "BT01", "BT02", "BT03", "BT04", "BT05", "BT06", "BT07",
    "BT08", "BT09", "BT10", "BT11", "BT12", "BT13", "BT14", "BT15"};

inline constexpr std::array<const char*, kCounterCount> kCounterLabels = {
    "ARIA", "AES", "CHACHA"};

struct StreamMetrics {
  double mPredictabilityScore = 0.0;
  double mUniformityScore = 0.0;
  double mRepetitivenessScore = 0.0;
  double mQualityScore = 0.0;
  double mEqualRate = 0.0;
  double mAbsCorrelation = 0.0;
  double mConditionalEntropyBits = 0.0;
  double mEntropyBits = 0.0;
  double mReducedChi2 = 0.0;
  double mWorstDeviationPercent = 0.0;
  double mRepeated64WindowPercent = 0.0;
  double mUnique16WindowPercent = 0.0;
  double mConsecutiveBlockEqualBytePercent = 0.0;
  int mLongestRunLength = 0;
};

struct PairTotals {
  std::uint64_t mExactMatchCount = 0U;
  std::uint64_t mEqualByteCount = 0U;
  std::uint64_t mEqualBitCount = 0U;
  std::uint64_t mHistogramOverlapCount = 0U;
  std::uint64_t mWindowOverlapCount = 0U;
  std::uint64_t mWindowTotalCount = 0U;
  std::uint64_t mTotalByteCount = 0U;
};

struct Window128 {
  std::uint64_t mLow = 0U;
  std::uint64_t mHigh = 0U;

  bool operator<(const Window128& pOther) const {
    if (mLow != pOther.mLow) {
      return mLow < pOther.mLow;
    }
    return mHigh < pOther.mHigh;
  }

  bool operator==(const Window128& pOther) const {
    return mLow == pOther.mLow && mHigh == pOther.mHigh;
  }
};

inline std::vector<unsigned char> BuildPasswordBytes(const std::string& pPassword) {
  return std::vector<unsigned char>(pPassword.begin(), pPassword.end());
}

inline bool TryGetDataLength(int pBlockCount, int* pDataLength) {
  if (pDataLength == nullptr || pBlockCount <= 0) {
    return false;
  }

  const std::uint64_t aLength =
      static_cast<std::uint64_t>(pBlockCount) * static_cast<std::uint64_t>(PASSWORD_EXPANDED_SIZE);
  if (aLength == 0U || aLength > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    return false;
  }

  *pDataLength = static_cast<int>(aLength);
  return true;
}

inline std::string HtmlEscape(std::string_view pText) {
  std::string aOut;
  aOut.reserve(pText.size());
  for (char aChar : pText) {
    switch (aChar) {
      case '&':
        aOut += "&amp;";
        break;
      case '<':
        aOut += "&lt;";
        break;
      case '>':
        aOut += "&gt;";
        break;
      case '"':
        aOut += "&quot;";
        break;
      case '\'':
        aOut += "&#39;";
        break;
      default:
        aOut.push_back(aChar);
        break;
    }
  }
  return aOut;
}

inline std::string HeatColor(double pPercent, bool pDiagonal) {
  if (pDiagonal) {
    return "#1f2937";
  }
  const double aClamped = std::max(0.0, std::min(100.0, pPercent));
  const int aRed = 255;
  const int aBlueGreen = static_cast<int>(255.0 - (aClamped * 1.9));
  std::ostringstream aStream;
  aStream << '#'
          << std::hex << std::setfill('0') << std::setw(2) << aRed
          << std::setw(2) << std::max(0, aBlueGreen)
          << std::setw(2) << std::max(0, aBlueGreen);
  return aStream.str();
}

inline unsigned int Popcount8(unsigned char pValue) {
  unsigned int aCount = 0U;
  unsigned int aValue = static_cast<unsigned int>(pValue);
  while (aValue != 0U) {
    aCount += (aValue & 1U);
    aValue >>= 1U;
  }
  return aCount;
}

inline void BuildHistogram(const std::vector<unsigned char>& pBytes, std::array<std::uint32_t, 256>* pHistogram) {
  pHistogram->fill(0U);
  for (unsigned char aByte : pBytes) {
    ++(*pHistogram)[aByte];
  }
}

inline std::uint64_t HistogramOverlap(const std::array<std::uint32_t, 256>& pLeft,
                                      const std::array<std::uint32_t, 256>& pRight) {
  std::uint64_t aOverlap = 0U;
  for (int aIndex = 0; aIndex < 256; ++aIndex) {
    aOverlap += static_cast<std::uint64_t>(std::min(pLeft[aIndex], pRight[aIndex]));
  }
  return aOverlap;
}

inline void BuildWindowKeys(const std::vector<unsigned char>& pBytes, std::vector<Window128>* pWindows) {
  pWindows->clear();
  if (pBytes.size() < static_cast<std::size_t>(kWindow16Bytes)) {
    return;
  }

  pWindows->reserve(pBytes.size() - static_cast<std::size_t>(kWindow16Bytes - 1));
  for (std::size_t aIndex = 0; aIndex + static_cast<std::size_t>(kWindow16Bytes) <= pBytes.size(); ++aIndex) {
    Window128 aKey{};
    std::memcpy(&aKey.mLow, pBytes.data() + aIndex, sizeof(std::uint64_t));
    std::memcpy(&aKey.mHigh, pBytes.data() + aIndex + 8U, sizeof(std::uint64_t));
    pWindows->push_back(aKey);
  }
  std::sort(pWindows->begin(), pWindows->end());
}

inline std::uint64_t WindowOverlapCount(const std::vector<Window128>& pLeft, const std::vector<Window128>& pRight) {
  std::size_t aLeft = 0U;
  std::size_t aRight = 0U;
  std::uint64_t aCommon = 0U;

  while (aLeft < pLeft.size() && aRight < pRight.size()) {
    if (pLeft[aLeft] == pRight[aRight]) {
      ++aCommon;
      ++aLeft;
      ++aRight;
    } else if (pLeft[aLeft] < pRight[aRight]) {
      ++aLeft;
    } else {
      ++aRight;
    }
  }

  return aCommon;
}

inline std::uint64_t HashWindow64(const unsigned char* pBytes) {
  std::uint64_t aHash = 1469598103934665603ULL;
  for (int aIndex = 0; aIndex < kWindow64Bytes; ++aIndex) {
    aHash ^= static_cast<std::uint64_t>(pBytes[aIndex]);
    aHash *= 1099511628211ULL;
  }
  return aHash;
}

template <typename TCounter>
void GenerateCounterStreamBytes(const unsigned char* pPassword,
                                int pPasswordLength,
                                std::vector<unsigned char>* pOutput) {
  TCounter aCounter;
  aCounter.Seed(const_cast<unsigned char*>(pPassword), pPasswordLength);
  aCounter.Get(pOutput->data(), static_cast<int>(pOutput->size()));
}

inline bool GenerateCounterStream(int pCounterIndex,
                                  const std::vector<unsigned char>& pPassword,
                                  int pDataLength,
                                  std::vector<unsigned char>* pOutput) {
  if (pOutput == nullptr || pDataLength <= 0) {
    return false;
  }

  pOutput->assign(static_cast<std::size_t>(pDataLength), 0U);
  unsigned char* aPasswordPtr = pPassword.empty() ? nullptr : const_cast<unsigned char*>(pPassword.data());
  const int aPasswordLength = static_cast<int>(pPassword.size());

  switch (pCounterIndex) {
    case 0:
      GenerateCounterStreamBytes<ARIA256Counter>(aPasswordPtr, aPasswordLength, pOutput);
      return true;
    case 1:
      GenerateCounterStreamBytes<AESCounter>(aPasswordPtr, aPasswordLength, pOutput);
      return true;
    case 2:
      GenerateCounterStreamBytes<ChaCha20Counter>(aPasswordPtr, aPasswordLength, pOutput);
      return true;
    default:
      return false;
  }
}

inline bool GenerateTwisterStream(int pTwisterIndex,
                                  const std::vector<unsigned char>& pPassword,
                                  int pDataLength,
                                  std::vector<unsigned char>* pOutput) {
  if (pOutput == nullptr || pDataLength <= 0 || pTwisterIndex < 0 || pTwisterIndex >= kTwisterCount) {
    return false;
  }

  pOutput->assign(static_cast<std::size_t>(pDataLength), 0U);
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  unsigned char* aPasswordPtr = pPassword.empty() ? nullptr : const_cast<unsigned char*>(pPassword.data());
  PasswordExpander::ExpandPasswordBlocks(static_cast<PasswordExpander::Type>(pTwisterIndex),
                                         aPasswordPtr,
                                         static_cast<unsigned int>(pPassword.size()),
                                         aWorker.data(),
                                         pOutput->data(),
                                         static_cast<unsigned int>(pDataLength));
  return true;
}

inline StreamMetrics AnalyzeStream(const std::vector<unsigned char>& pBytes) {
  StreamMetrics aMetrics;
  if (pBytes.empty()) {
    return aMetrics;
  }

  constexpr int kAlphabet = 256;
  constexpr double kExpectedEqualRate = 1.0 / 256.0;

  std::array<std::uint64_t, kAlphabet * kAlphabet> aTransitions = {};
  std::array<std::uint64_t, kAlphabet> aRowTotals = {};
  std::array<std::uint64_t, kAlphabet> aHistogram = {};

  std::uint64_t aPairCount = 0U;
  std::uint64_t aEqualPairs = 0U;
  long double aSumX = 0.0L;
  long double aSumY = 0.0L;
  long double aSumXX = 0.0L;
  long double aSumYY = 0.0L;
  long double aSumXY = 0.0L;

  int aLongestRun = 1;
  int aCurrentRun = 1;

  for (std::size_t aIndex = 0; aIndex < pBytes.size(); ++aIndex) {
    const unsigned char aByte = pBytes[aIndex];
    ++aHistogram[aByte];

    if (aIndex > 0U) {
      const unsigned char aPrev = pBytes[aIndex - 1U];
      const std::size_t aTransitionIndex =
          static_cast<std::size_t>((static_cast<std::uint64_t>(aPrev) << 8U) | static_cast<std::uint64_t>(aByte));
      ++aTransitions[aTransitionIndex];
      ++aRowTotals[aPrev];
      ++aPairCount;
      if (aPrev == aByte) {
        ++aEqualPairs;
      }

      const long double aX = static_cast<long double>(aPrev);
      const long double aY = static_cast<long double>(aByte);
      aSumX += aX;
      aSumY += aY;
      aSumXX += aX * aX;
      aSumYY += aY * aY;
      aSumXY += aX * aY;

      if (aPrev == aByte) {
        ++aCurrentRun;
      } else {
        aLongestRun = std::max(aLongestRun, aCurrentRun);
        aCurrentRun = 1;
      }
    }
  }
  aLongestRun = std::max(aLongestRun, aCurrentRun);
  aMetrics.mLongestRunLength = aLongestRun;

  const double aTotalBytes = static_cast<double>(pBytes.size());
  const double aExpected = aTotalBytes / static_cast<double>(kAlphabet);
  double aChi2 = 0.0;
  double aEntropy = 0.0;
  double aWorstDeviationPercent = 0.0;
  for (int aByte = 0; aByte < kAlphabet; ++aByte) {
    const double aObserved = static_cast<double>(aHistogram[static_cast<std::size_t>(aByte)]);
    const double aDelta = aObserved - aExpected;
    aChi2 += (aDelta * aDelta) / aExpected;
    const double aDeviationPercent = (std::abs(aDelta) / aExpected) * 100.0;
    aWorstDeviationPercent = std::max(aWorstDeviationPercent, aDeviationPercent);
    if (aObserved > 0.0) {
      const double aP = aObserved / aTotalBytes;
      aEntropy -= aP * std::log2(aP);
    }
  }

  double aConditionalEntropy = 0.0;
  if (aPairCount > 0U) {
    const double aPairCountD = static_cast<double>(aPairCount);
    for (int aRow = 0; aRow < kAlphabet; ++aRow) {
      const std::uint64_t aRowCount = aRowTotals[static_cast<std::size_t>(aRow)];
      if (aRowCount == 0U) {
        continue;
      }
      const double aRowCountD = static_cast<double>(aRowCount);
      for (int aCol = 0; aCol < kAlphabet; ++aCol) {
        const std::uint64_t aCount = aTransitions[static_cast<std::size_t>((aRow << 8) | aCol)];
        if (aCount == 0U) {
          continue;
        }
        const double aCountD = static_cast<double>(aCount);
        const double aPxy = aCountD / aPairCountD;
        const double aPyGivenX = aCountD / aRowCountD;
        aConditionalEntropy -= aPxy * std::log2(aPyGivenX);
      }
    }
  }

  double aCorrelation = 0.0;
  if (aPairCount > 0U) {
    const long double aN = static_cast<long double>(aPairCount);
    const long double aNumerator = (aN * aSumXY) - (aSumX * aSumY);
    const long double aDenomA = (aN * aSumXX) - (aSumX * aSumX);
    const long double aDenomB = (aN * aSumYY) - (aSumY * aSumY);
    if (aDenomA > 0.0L && aDenomB > 0.0L) {
      aCorrelation = static_cast<double>(aNumerator / std::sqrt(aDenomA * aDenomB));
    }
  }

  double aRepeated64WindowPercent = 0.0;
  const std::size_t aWindow64Count = pBytes.size() / static_cast<std::size_t>(kWindow64Bytes);
  if (aWindow64Count > 0U) {
    std::unordered_set<std::uint64_t> aSeen;
    aSeen.reserve(aWindow64Count * 2U);
    std::size_t aDuplicateCount = 0U;
    for (std::size_t aIndex = 0U; aIndex < aWindow64Count; ++aIndex) {
      const std::uint64_t aHash = HashWindow64(
          pBytes.data() + (aIndex * static_cast<std::size_t>(kWindow64Bytes)));
      if (!aSeen.insert(aHash).second) {
        ++aDuplicateCount;
      }
    }
    aRepeated64WindowPercent =
        100.0 * static_cast<double>(aDuplicateCount) / static_cast<double>(aWindow64Count);
  }

  double aUnique16WindowPercent = 100.0;
  if (pBytes.size() >= static_cast<std::size_t>(kWindow16Bytes)) {
    std::vector<Window128> aWindows;
    BuildWindowKeys(pBytes, &aWindows);
    const auto aUniqueEnd = std::unique(aWindows.begin(), aWindows.end());
    const std::size_t aUniqueCount = static_cast<std::size_t>(std::distance(aWindows.begin(), aUniqueEnd));
    aUnique16WindowPercent =
        100.0 * static_cast<double>(aUniqueCount) / static_cast<double>(aWindows.size());
  }

  double aConsecutiveBlockEqualBytePercent = 0.0;
  if (pBytes.size() >= (static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE) * 2U) &&
      (pBytes.size() % static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE)) == 0U) {
    std::uint64_t aEqual = 0U;
    std::uint64_t aCompared = 0U;
    const std::size_t aBlockLength = static_cast<std::size_t>(PASSWORD_EXPANDED_SIZE);
    for (std::size_t aOffset = aBlockLength; aOffset < pBytes.size(); aOffset += aBlockLength) {
      for (std::size_t aIndex = 0U; aIndex < aBlockLength; ++aIndex) {
        if (pBytes[aOffset + aIndex] == pBytes[(aOffset - aBlockLength) + aIndex]) {
          ++aEqual;
        }
        ++aCompared;
      }
    }
    aConsecutiveBlockEqualBytePercent =
        100.0 * static_cast<double>(aEqual) / static_cast<double>(aCompared);
  }

  const double aEqualRate = (aPairCount == 0U) ? 0.0
      : static_cast<double>(aEqualPairs) / static_cast<double>(aPairCount);
  const double aPredictabilityBits = std::max(0.0, 8.0 - aConditionalEntropy);

  const double aPenaltyEqualRate = std::abs(aEqualRate - kExpectedEqualRate) * 25600.0;
  const double aPenaltyCorrelation = std::abs(aCorrelation) * 250.0;
  const double aPenaltyPredictabilityBits = aPredictabilityBits * 45.0;
  double aPredictabilityScore = 100.0 - (aPenaltyEqualRate + aPenaltyCorrelation + aPenaltyPredictabilityBits);
  aPredictabilityScore = std::max(0.0, std::min(100.0, aPredictabilityScore));

  const double aReducedChi2 = aChi2 / 255.0;
  const double aPenaltyChi2 = std::abs(aReducedChi2 - 1.0) * 35.0;
  const double aPenaltyDeviation = aWorstDeviationPercent * 0.9;
  const double aPenaltyEntropy = std::max(0.0, 8.0 - aEntropy) * 60.0;
  double aUniformityScore = 100.0 - (aPenaltyChi2 + aPenaltyDeviation + aPenaltyEntropy);
  aUniformityScore = std::max(0.0, std::min(100.0, aUniformityScore));

  const double aPenaltyRepeatedWindows = aRepeated64WindowPercent * 2.4;
  const double aPenaltyUniqueWindows = (100.0 - aUnique16WindowPercent) * 1.25;
  const double aPenaltyConsecutiveBlocks = aConsecutiveBlockEqualBytePercent * 14.0;
  const double aPenaltyLongestRun = std::max(0, aLongestRun - 4) * 4.0;
  double aRepetitivenessScore =
      100.0 - (aPenaltyRepeatedWindows + aPenaltyUniqueWindows + aPenaltyConsecutiveBlocks + aPenaltyLongestRun);
  aRepetitivenessScore = std::max(0.0, std::min(100.0, aRepetitivenessScore));

  aMetrics.mPredictabilityScore = aPredictabilityScore;
  aMetrics.mUniformityScore = aUniformityScore;
  aMetrics.mRepetitivenessScore = aRepetitivenessScore;
  aMetrics.mQualityScore = (aPredictabilityScore + aUniformityScore + aRepetitivenessScore) / 3.0;
  aMetrics.mEqualRate = aEqualRate;
  aMetrics.mAbsCorrelation = std::abs(aCorrelation);
  aMetrics.mConditionalEntropyBits = aConditionalEntropy;
  aMetrics.mEntropyBits = aEntropy;
  aMetrics.mReducedChi2 = aReducedChi2;
  aMetrics.mWorstDeviationPercent = aWorstDeviationPercent;
  aMetrics.mRepeated64WindowPercent = aRepeated64WindowPercent;
  aMetrics.mUnique16WindowPercent = aUnique16WindowPercent;
  aMetrics.mConsecutiveBlockEqualBytePercent = aConsecutiveBlockEqualBytePercent;
  return aMetrics;
}

inline double CompareMetricsGrade(const StreamMetrics& pLeft, const StreamMetrics& pRight) {
  double aScore = 100.0;
  aScore -= std::abs(pLeft.mQualityScore - pRight.mQualityScore) * 0.30;
  aScore -= std::abs(pLeft.mPredictabilityScore - pRight.mPredictabilityScore) * 0.25;
  aScore -= std::abs(pLeft.mUniformityScore - pRight.mUniformityScore) * 0.20;
  aScore -= std::abs(pLeft.mRepetitivenessScore - pRight.mRepetitivenessScore) * 0.15;
  aScore -= std::abs(pLeft.mEntropyBits - pRight.mEntropyBits) * 35.0;
  aScore -= std::abs(pLeft.mConditionalEntropyBits - pRight.mConditionalEntropyBits) * 10.0;
  aScore -= std::abs(pLeft.mConsecutiveBlockEqualBytePercent - pRight.mConsecutiveBlockEqualBytePercent) * 2.0;
  return std::max(0.0, std::min(100.0, aScore));
}

inline void ComputeTwisterConfusionMatrices(
    const std::array<std::vector<unsigned char>, kTwisterCount>& pStreams,
    int pLoops,
    std::array<std::array<PairTotals, kTwisterCount>, kTwisterCount>* pTotals,
    std::array<std::array<double, kTwisterCount>, kTwisterCount>* pByteEqualityPercent,
    std::array<std::array<double, kTwisterCount>, kTwisterCount>* pBitEqualityPercent,
    std::array<std::array<double, kTwisterCount>, kTwisterCount>* pWindowOverlapPercent,
    std::array<std::array<double, kTwisterCount>, kTwisterCount>* pHistogramOverlapPercent) {
  if (pTotals == nullptr || pByteEqualityPercent == nullptr || pBitEqualityPercent == nullptr ||
      pWindowOverlapPercent == nullptr || pHistogramOverlapPercent == nullptr) {
    return;
  }

  pTotals->fill({});
  pByteEqualityPercent->fill({});
  pBitEqualityPercent->fill({});
  pWindowOverlapPercent->fill({});
  pHistogramOverlapPercent->fill({});

  std::array<std::array<std::uint32_t, 256>, kTwisterCount> aHistograms = {};
  std::array<std::vector<Window128>, kTwisterCount> aWindows = {};
  for (int aIndex = 0; aIndex < kTwisterCount; ++aIndex) {
    BuildHistogram(pStreams[aIndex], &aHistograms[aIndex]);
    BuildWindowKeys(pStreams[aIndex], &aWindows[aIndex]);
  }

  for (int aLeft = 0; aLeft < kTwisterCount; ++aLeft) {
    for (int aRight = aLeft; aRight < kTwisterCount; ++aRight) {
      PairTotals aPair{};
      aPair.mExactMatchCount = (pStreams[aLeft] == pStreams[aRight]) ? static_cast<std::uint64_t>(pLoops) : 0U;

      for (std::size_t aByteIndex = 0U; aByteIndex < pStreams[aLeft].size(); ++aByteIndex) {
        const unsigned char aByteA = pStreams[aLeft][aByteIndex];
        const unsigned char aByteB = pStreams[aRight][aByteIndex];
        if (aByteA == aByteB) {
          ++aPair.mEqualByteCount;
        }
        aPair.mEqualBitCount += static_cast<std::uint64_t>(8U - Popcount8(static_cast<unsigned char>(aByteA ^ aByteB)));
      }

      aPair.mHistogramOverlapCount = HistogramOverlap(aHistograms[aLeft], aHistograms[aRight]);
      aPair.mWindowOverlapCount = WindowOverlapCount(aWindows[aLeft], aWindows[aRight]);
      aPair.mWindowTotalCount = static_cast<std::uint64_t>(aWindows[aLeft].size());
      aPair.mTotalByteCount = static_cast<std::uint64_t>(pStreams[aLeft].size());
      (*pTotals)[aLeft][aRight] = aPair;
      (*pTotals)[aRight][aLeft] = aPair;
    }
  }

  for (int aRow = 0; aRow < kTwisterCount; ++aRow) {
    for (int aCol = 0; aCol < kTwisterCount; ++aCol) {
      const PairTotals& aPair = (*pTotals)[aRow][aCol];
      if (aPair.mTotalByteCount == 0U) {
        continue;
      }
      const double aTotalBytes = static_cast<double>(aPair.mTotalByteCount);
      (*pByteEqualityPercent)[aRow][aCol] = 100.0 * static_cast<double>(aPair.mEqualByteCount) / aTotalBytes;
      (*pBitEqualityPercent)[aRow][aCol] =
          100.0 * static_cast<double>(aPair.mEqualBitCount) / (aTotalBytes * 8.0);
      (*pWindowOverlapPercent)[aRow][aCol] =
          (aPair.mWindowTotalCount == 0U)
              ? 0.0
              : 100.0 * static_cast<double>(aPair.mWindowOverlapCount) / static_cast<double>(aPair.mWindowTotalCount);
      (*pHistogramOverlapPercent)[aRow][aCol] =
          100.0 * static_cast<double>(aPair.mHistogramOverlapCount) / aTotalBytes;
    }
  }
}

}  // namespace peanutbutter::tests::password_expander_quality

#endif  // BREAD_TESTS_COMMON_PASSWORD_EXPANDER_QUALITY_UTILS_HPP_
