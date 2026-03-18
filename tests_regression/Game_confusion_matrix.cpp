#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <string_view>
#include <vector>

#include "src/Tables/counters/ChaCha20Counter.hpp"
#include "tests/common/GameCatalog.hpp"
#include "tests/common/GameChunkRunner.hpp"
#include "tests/common/Tests.hpp"

namespace {

constexpr int kGameCount = peanutbutter::games::GameBoard::kGameCount;

struct PairTotals {
  std::uint64_t mExactMatchCount = 0U;
  std::uint64_t mEqualByteCount = 0U;
  std::uint64_t mEqualBitCount = 0U;
  std::uint64_t mHistogramOverlapCount = 0U;
  std::uint64_t mWindowOverlapCount = 0U;
  std::uint64_t mWindowTotalCount = 0U;
  std::uint64_t mStableLcsCount = 0U;
  std::uint64_t mAdjacencyPreservedCount = 0U;
  std::uint64_t mAdjacencyTotalCount = 0U;
  double mPositionRetentionSum = 0.0;
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

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = peanutbutter::tests::config::ApplyGlobalSeed(pSalt);
  return 0xA24CAED4U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B4U);
}

void FillInput(std::vector<unsigned char>* pBytes, int pTrial) {
  std::uint32_t aState = SeedState(pTrial + static_cast<int>(pBytes->size()));
  for (std::size_t aIndex = 0; aIndex < pBytes->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pBytes)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

const char* GameAbbreviation(int pIndex) {
  static constexpr const char* kAbbreviations[kGameCount] = {
      "SSG", "SLG", "ISG", "ILG", "STG", "ITG", "SSR", "SLR",
      "ISR", "ILR", "STR", "ITR", "SSF", "SLF", "ISF", "ILF"};
  return (pIndex >= 0 && pIndex < kGameCount) ? kAbbreviations[pIndex] : "???";
}

void BuildHistogram(const std::vector<unsigned char>& pBytes, std::array<std::uint32_t, 256>* pHistogram) {
  pHistogram->fill(0U);
  for (unsigned char aByte : pBytes) {
    ++(*pHistogram)[aByte];
  }
}

unsigned int Popcount8(unsigned char pValue) {
  unsigned int aCount = 0U;
  unsigned int aValue = static_cast<unsigned int>(pValue);
  while (aValue != 0U) {
    aCount += (aValue & 1U);
    aValue >>= 1U;
  }
  return aCount;
}

std::uint64_t HistogramOverlap(const std::array<std::uint32_t, 256>& pLeft,
                               const std::array<std::uint32_t, 256>& pRight) {
  std::uint64_t aOverlap = 0U;
  for (int aIndex = 0; aIndex < 256; ++aIndex) {
    aOverlap += static_cast<std::uint64_t>(std::min(pLeft[aIndex], pRight[aIndex]));
  }
  return aOverlap;
}

void BuildStableOccurrenceTokens(const std::vector<unsigned char>& pBytes, std::vector<std::uint32_t>* pTokens) {
  pTokens->clear();
  pTokens->reserve(pBytes.size());
  std::array<std::uint32_t, 256> aSeen = {};
  for (unsigned char aByte : pBytes) {
    const std::uint32_t aOrdinal = aSeen[aByte]++;
    pTokens->push_back(static_cast<std::uint32_t>(aByte) | (aOrdinal << 8U));
  }
}

void BuildWindowKeys(const std::vector<unsigned char>& pBytes, std::vector<Window128>* pWindows) {
  pWindows->clear();
  if (pBytes.size() < 16U) {
    return;
  }

  pWindows->reserve(pBytes.size() - 15U);
  for (std::size_t aIndex = 0; aIndex + 16U <= pBytes.size(); ++aIndex) {
    Window128 aKey{};
    std::memcpy(&aKey.mLow, pBytes.data() + aIndex, sizeof(std::uint64_t));
    std::memcpy(&aKey.mHigh, pBytes.data() + aIndex + 8U, sizeof(std::uint64_t));
    pWindows->push_back(aKey);
  }
  std::sort(pWindows->begin(), pWindows->end());
}

std::uint64_t WindowOverlapCount(const std::vector<Window128>& pLeft, const std::vector<Window128>& pRight) {
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

int StableOccurrenceLcsApprox(const std::vector<std::uint32_t>& pLeft, const std::vector<std::uint32_t>& pRight) {
  std::unordered_map<std::uint32_t, int> aRightPositions;
  aRightPositions.reserve(pRight.size() * 2U);
  for (int aIndex = 0; aIndex < static_cast<int>(pRight.size()); ++aIndex) {
    aRightPositions.emplace(pRight[static_cast<std::size_t>(aIndex)], aIndex);
  }

  std::vector<int> aLanes;
  aLanes.reserve(pLeft.size());
  for (std::uint32_t aToken : pLeft) {
    const auto aFound = aRightPositions.find(aToken);
    if (aFound == aRightPositions.end()) {
      continue;
    }
    const int aPosition = aFound->second;
    auto aIt = std::lower_bound(aLanes.begin(), aLanes.end(), aPosition);
    if (aIt == aLanes.end()) {
      aLanes.push_back(aPosition);
    } else {
      *aIt = aPosition;
    }
  }

  return static_cast<int>(aLanes.size());
}

std::uint64_t OrderedAdjacencyPreservedCount(const std::vector<std::uint32_t>& pLeft,
                                             const std::vector<std::uint32_t>& pRight) {
  if (pLeft.size() < 2U || pRight.size() < 2U) {
    return 0U;
  }

  std::unordered_map<std::uint32_t, int> aRightPositions;
  aRightPositions.reserve(pRight.size() * 2U);
  for (int aIndex = 0; aIndex < static_cast<int>(pRight.size()); ++aIndex) {
    aRightPositions.emplace(pRight[static_cast<std::size_t>(aIndex)], aIndex);
  }

  std::uint64_t aCount = 0U;
  for (int aIndex = 0; aIndex + 1 < static_cast<int>(pLeft.size()); ++aIndex) {
    const auto aFoundA = aRightPositions.find(pLeft[static_cast<std::size_t>(aIndex)]);
    const auto aFoundB = aRightPositions.find(pLeft[static_cast<std::size_t>(aIndex + 1)]);
    if (aFoundA != aRightPositions.end() && aFoundB != aRightPositions.end() &&
        aFoundB->second == (aFoundA->second + 1)) {
      ++aCount;
    }
  }
  return aCount;
}

double PositionRetentionPercent(const std::vector<std::uint32_t>& pLeft, const std::vector<std::uint32_t>& pRight) {
  if (pLeft.empty() || pRight.empty() || pLeft.size() != pRight.size()) {
    return 0.0;
  }

  std::unordered_map<std::uint32_t, int> aRightPositions;
  aRightPositions.reserve(pRight.size() * 2U);
  for (int aIndex = 0; aIndex < static_cast<int>(pRight.size()); ++aIndex) {
    aRightPositions.emplace(pRight[static_cast<std::size_t>(aIndex)], aIndex);
  }

  std::uint64_t aDisplacementSum = 0U;
  for (int aIndex = 0; aIndex < static_cast<int>(pLeft.size()); ++aIndex) {
    const auto aFound = aRightPositions.find(pLeft[static_cast<std::size_t>(aIndex)]);
    if (aFound == aRightPositions.end()) {
      continue;
    }
    const int aDelta = std::abs(aIndex - aFound->second);
    aDisplacementSum += static_cast<std::uint64_t>(aDelta);
  }

  const double aLength = static_cast<double>(pLeft.size());
  const double aMaxDisp = (aLength <= 1.0) ? 1.0 : (aLength - 1.0);
  const double aMeanDisp = static_cast<double>(aDisplacementSum) / aLength;
  return 100.0 * std::max(0.0, 1.0 - (aMeanDisp / aMaxDisp));
}

void PrintPercentMatrix(const char* pTitle,
                        const std::array<std::array<double, kGameCount>, kGameCount>& pMatrix) {
  std::cout << "\n[MATRIX] " << pTitle << "\n";
  std::cout << std::setw(6) << "";
  for (int aCol = 0; aCol < kGameCount; ++aCol) {
    std::cout << std::setw(6) << GameAbbreviation(aCol);
  }
  std::cout << "\n";

  for (int aRow = 0; aRow < kGameCount; ++aRow) {
    std::cout << std::setw(6) << GameAbbreviation(aRow);
    for (int aCol = 0; aCol < kGameCount; ++aCol) {
      std::cout << std::setw(6) << static_cast<int>(pMatrix[aRow][aCol] + 0.5);
    }
    std::cout << "\n";
  }
}

std::string HeatColor(double pPercent, bool pDiagonal) {
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

void WriteHtmlMatrix(std::ofstream& pOut,
                     const char* pTitle,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pMatrix) {
  pOut << "<section>\n";
  pOut << "<h2>" << pTitle << "</h2>\n";
  pOut << "<table>\n";
  pOut << "<tr><th></th>";
  for (int aCol = 0; aCol < kGameCount; ++aCol) {
    pOut << "<th>" << GameAbbreviation(aCol) << "</th>";
  }
  pOut << "</tr>\n";

  for (int aRow = 0; aRow < kGameCount; ++aRow) {
    pOut << "<tr><th>" << GameAbbreviation(aRow) << "</th>";
    for (int aCol = 0; aCol < kGameCount; ++aCol) {
      const bool aDiagonal = (aRow == aCol);
      const double aValue = pMatrix[aRow][aCol];
      const std::string aColor = HeatColor(aValue, aDiagonal);
      const char* aTextColor = (aDiagonal || aValue >= 55.0) ? "#ffffff" : "#111827";
      pOut << "<td style=\"background:" << aColor << ";color:" << aTextColor << "\">"
           << std::fixed << std::setprecision(1) << aValue << "</td>";
    }
    pOut << "</tr>\n";
  }
  pOut << "</table>\n";
  pOut << "</section>\n";
}

void WriteHtmlReport(const char* pPath,
                     int pLoops,
                     int pDataLength,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pByteEqualityPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pBitEqualityPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pWindowOverlapPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pStableLcsPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pAdjacencyPreservedPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pPositionRetentionPercent,
                     const std::array<std::array<double, kGameCount>, kGameCount>& pHistogramOverlapPercent,
                     const std::array<std::array<PairTotals, kGameCount>, kGameCount>& pTotals) {
  std::ofstream aOut(pPath, std::ios::out | std::ios::trunc);
  if (!aOut.is_open()) {
    return;
  }

  aOut << "<!doctype html>\n<html><head><meta charset=\"utf-8\">";
  aOut << "<title>Game Confusion Matrix</title>";
  aOut << "<style>"
          "body{font-family:Menlo,Consolas,monospace;background:#f8fafc;color:#111827;padding:24px;}"
          "h1,h2{margin:0 0 12px 0;} section{margin-top:24px;}"
          "table{border-collapse:collapse;margin-top:8px;}"
          "th,td{border:1px solid #cbd5e1;padding:6px 8px;text-align:center;font-size:12px;}"
          "th{background:#e2e8f0;} .meta{margin:12px 0 20px 0;} .pair{margin:4px 0;}"
          "</style></head><body>\n";
  aOut << "<h1>Game Confusion Matrix</h1>\n";
  aOut << "<div class=\"meta\">loops=" << pLoops << " bytes=" << pDataLength << " games=" << kGameCount << "</div>\n";
  aOut << "<section><h2>Legend</h2>\n";
  for (int aIndex = 0; aIndex < kGameCount; ++aIndex) {
    const char* aName = peanutbutter::tests::games::GetGameName(
        peanutbutter::tests::games::kAllGames[static_cast<std::size_t>(aIndex)].mGameIndex);
    aOut << "<div>" << GameAbbreviation(aIndex) << " = " << ((aName != nullptr) ? aName : "UNKNOWN") << "</div>\n";
  }
  aOut << "</section>\n";

  WriteHtmlMatrix(aOut, "Byte Equality Percent", pByteEqualityPercent);
  WriteHtmlMatrix(aOut, "Bit Equality Percent", pBitEqualityPercent);
  WriteHtmlMatrix(aOut, "16-Byte Window Overlap Percent", pWindowOverlapPercent);
  WriteHtmlMatrix(aOut, "Stable LCS Approx Percent", pStableLcsPercent);
  WriteHtmlMatrix(aOut, "Ordered Adjacency Preserved Percent", pAdjacencyPreservedPercent);
  WriteHtmlMatrix(aOut, "Position Retention Percent", pPositionRetentionPercent);
  WriteHtmlMatrix(aOut, "Histogram Overlap Percent", pHistogramOverlapPercent);

  aOut << "<section><h2>Nearest Neighbors By Byte Equality</h2>\n";
  for (int aRow = 0; aRow < kGameCount; ++aRow) {
    int aBestIndex = -1;
    double aBestScore = -1.0;
    for (int aCol = 0; aCol < kGameCount; ++aCol) {
      if (aRow == aCol) {
        continue;
      }
      if (pByteEqualityPercent[aRow][aCol] > aBestScore) {
        aBestScore = pByteEqualityPercent[aRow][aCol];
        aBestIndex = aCol;
      }
    }
    aOut << "<div class=\"pair\">" << GameAbbreviation(aRow) << " -> " << GameAbbreviation(aBestIndex)
         << " | byte=" << std::fixed << std::setprecision(2) << aBestScore
         << " | bit=" << pBitEqualityPercent[aRow][aBestIndex]
         << " | lcs=" << pStableLcsPercent[aRow][aBestIndex]
         << " | adj=" << pAdjacencyPreservedPercent[aRow][aBestIndex]
         << " | retain=" << pPositionRetentionPercent[aRow][aBestIndex]
         << " | hist=" << pHistogramOverlapPercent[aRow][aBestIndex]
         << " | exact=" << pTotals[aRow][aBestIndex].mExactMatchCount << "/" << pLoops
         << "</div>\n";
  }
  aOut << "</section>\n";
  aOut << "</body></html>\n";
}

}  // namespace

int main() {
  const int aLoops =
      (peanutbutter::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : peanutbutter::tests::config::TEST_LOOP_COUNT;
  const int aDataLength =
      (peanutbutter::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : peanutbutter::tests::config::GAME_TEST_DATA_LENGTH;

  if (!peanutbutter::tests::games::IsValidGameByteLength(aDataLength)) {
    std::cerr << "[FAIL] invalid game length for confusion matrix: " << aDataLength << "\n";
    return 1;
  }

  std::array<std::array<PairTotals, kGameCount>, kGameCount> aTotals = {};

  std::cout << "[INFO] game confusion matrix"
            << " loops=" << aLoops
            << " bytes=" << aDataLength
            << " games=" << kGameCount << "\n";

  for (int aIndex = 0; aIndex < kGameCount; ++aIndex) {
    const char* aName = peanutbutter::tests::games::GetGameName(
        peanutbutter::tests::games::kAllGames[static_cast<std::size_t>(aIndex)].mGameIndex);
    std::cout << "[INFO] " << GameAbbreviation(aIndex) << " = " << ((aName != nullptr) ? aName : "UNKNOWN") << "\n";
  }

  for (int aLoop = 0; aLoop < aLoops; ++aLoop) {
    std::vector<unsigned char> aInput(static_cast<std::size_t>(aDataLength), 0U);
    FillInput(&aInput, aLoop);

    std::array<std::vector<unsigned char>, kGameCount> aOutputs = {};
    std::array<std::array<std::uint32_t, 256>, kGameCount> aHistograms = {};
    std::array<std::vector<std::uint32_t>, kGameCount> aTokens = {};
    std::array<std::vector<Window128>, kGameCount> aWindows = {};

    for (int aGameIndex = 0; aGameIndex < kGameCount; ++aGameIndex) {
      aOutputs[aGameIndex].assign(static_cast<std::size_t>(aDataLength), 0U);
      peanutbutter::tests::games::GameRunSummary aSummary;
      if (!peanutbutter::tests::games::RunGameChunksFromInput<ChaCha20Counter>(
              peanutbutter::tests::games::kAllGames[static_cast<std::size_t>(aGameIndex)].mGameIndex,
              aInput.data(),
              aDataLength,
              aOutputs[aGameIndex].data(),
              &aSummary)) {
        std::cerr << "[FAIL] game confusion execution failed for game=" << aGameIndex << " loop=" << aLoop << "\n";
        return 1;
      }
      BuildHistogram(aOutputs[aGameIndex], &aHistograms[aGameIndex]);
      BuildStableOccurrenceTokens(aOutputs[aGameIndex], &aTokens[aGameIndex]);
      BuildWindowKeys(aOutputs[aGameIndex], &aWindows[aGameIndex]);
    }

    for (int aLeft = 0; aLeft < kGameCount; ++aLeft) {
      for (int aRight = aLeft; aRight < kGameCount; ++aRight) {
        PairTotals& aPair = aTotals[aLeft][aRight];
        const bool aExactMatch = (aOutputs[aLeft] == aOutputs[aRight]);
        if (aExactMatch) {
          ++aPair.mExactMatchCount;
        }

        std::uint64_t aEqualBytes = 0U;
        std::uint64_t aEqualBits = 0U;
        for (int aByteIndex = 0; aByteIndex < aDataLength; ++aByteIndex) {
          const unsigned char aByteA = aOutputs[aLeft][static_cast<std::size_t>(aByteIndex)];
          const unsigned char aByteB = aOutputs[aRight][static_cast<std::size_t>(aByteIndex)];
          if (aByteA == aByteB) {
            ++aEqualBytes;
          }
          aEqualBits += static_cast<std::uint64_t>(8U - Popcount8(static_cast<unsigned char>(aByteA ^ aByteB)));
        }

        aPair.mEqualByteCount += aEqualBytes;
        aPair.mEqualBitCount += aEqualBits;
        aPair.mHistogramOverlapCount += HistogramOverlap(aHistograms[aLeft], aHistograms[aRight]);
        aPair.mWindowOverlapCount += WindowOverlapCount(aWindows[aLeft], aWindows[aRight]);
        aPair.mWindowTotalCount += static_cast<std::uint64_t>(aWindows[aLeft].size());
        aPair.mStableLcsCount += static_cast<std::uint64_t>(StableOccurrenceLcsApprox(aTokens[aLeft], aTokens[aRight]));
        aPair.mAdjacencyPreservedCount += OrderedAdjacencyPreservedCount(aTokens[aLeft], aTokens[aRight]);
        aPair.mAdjacencyTotalCount +=
            (aTokens[aLeft].size() > 1U) ? static_cast<std::uint64_t>(aTokens[aLeft].size() - 1U) : 0U;
        aPair.mPositionRetentionSum += PositionRetentionPercent(aTokens[aLeft], aTokens[aRight]);
        aPair.mTotalByteCount += static_cast<std::uint64_t>(aDataLength);

        if (aLeft != aRight) {
          aTotals[aRight][aLeft] = aPair;
        }
      }
    }
  }

  std::array<std::array<double, kGameCount>, kGameCount> aByteEqualityPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aBitEqualityPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aWindowOverlapPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aStableLcsPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aAdjacencyPreservedPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aPositionRetentionPercent = {};
  std::array<std::array<double, kGameCount>, kGameCount> aHistogramOverlapPercent = {};

  for (int aRow = 0; aRow < kGameCount; ++aRow) {
    for (int aCol = 0; aCol < kGameCount; ++aCol) {
      const PairTotals& aPair = aTotals[aRow][aCol];
      if (aPair.mTotalByteCount == 0U) {
        continue;
      }
      const double aTotalBytes = static_cast<double>(aPair.mTotalByteCount);
      aByteEqualityPercent[aRow][aCol] = 100.0 * static_cast<double>(aPair.mEqualByteCount) / aTotalBytes;
      aBitEqualityPercent[aRow][aCol] = 100.0 * static_cast<double>(aPair.mEqualBitCount) / (aTotalBytes * 8.0);
      aWindowOverlapPercent[aRow][aCol] =
          (aPair.mWindowTotalCount == 0U) ? 0.0
                                          : 100.0 * static_cast<double>(aPair.mWindowOverlapCount) /
                                                static_cast<double>(aPair.mWindowTotalCount);
      aStableLcsPercent[aRow][aCol] =
          100.0 * static_cast<double>(aPair.mStableLcsCount) / aTotalBytes;
      aAdjacencyPreservedPercent[aRow][aCol] =
          (aPair.mAdjacencyTotalCount == 0U) ? 0.0
                                             : 100.0 * static_cast<double>(aPair.mAdjacencyPreservedCount) /
                                                   static_cast<double>(aPair.mAdjacencyTotalCount);
      aPositionRetentionPercent[aRow][aCol] = aPair.mPositionRetentionSum / static_cast<double>(aLoops);
      aHistogramOverlapPercent[aRow][aCol] =
          100.0 * static_cast<double>(aPair.mHistogramOverlapCount) / aTotalBytes;
    }
  }

  PrintPercentMatrix("Byte Equality Percent", aByteEqualityPercent);
  PrintPercentMatrix("Bit Equality Percent", aBitEqualityPercent);
  PrintPercentMatrix("16-Byte Window Overlap Percent", aWindowOverlapPercent);
  PrintPercentMatrix("Stable LCS Approx Percent", aStableLcsPercent);
  PrintPercentMatrix("Ordered Adjacency Preserved Percent", aAdjacencyPreservedPercent);
  PrintPercentMatrix("Position Retention Percent", aPositionRetentionPercent);
  PrintPercentMatrix("Histogram Overlap Percent", aHistogramOverlapPercent);

  constexpr const char* kHtmlOutputPath = "game_confusion_matrix.html";
  WriteHtmlReport(kHtmlOutputPath,
                  aLoops,
                  aDataLength,
                  aByteEqualityPercent,
                  aBitEqualityPercent,
                  aWindowOverlapPercent,
                  aStableLcsPercent,
                  aAdjacencyPreservedPercent,
                  aPositionRetentionPercent,
                  aHistogramOverlapPercent,
                  aTotals);
  std::cout << "\n[INFO] wrote visual heatmap to " << kHtmlOutputPath << "\n";

  std::cout << "\n[SUMMARY] nearest neighbors by byte equality\n";
  for (int aRow = 0; aRow < kGameCount; ++aRow) {
    int aBestIndex = -1;
    double aBestScore = -1.0;
    for (int aCol = 0; aCol < kGameCount; ++aCol) {
      if (aRow == aCol) {
        continue;
      }
      if (aByteEqualityPercent[aRow][aCol] > aBestScore) {
        aBestScore = aByteEqualityPercent[aRow][aCol];
        aBestIndex = aCol;
      }
    }
    std::cout << "[PAIR] " << GameAbbreviation(aRow) << " -> " << GameAbbreviation(aBestIndex)
              << " byte_equal_pct=" << std::fixed << std::setprecision(2) << aBestScore
              << " bit_equal_pct=" << aBitEqualityPercent[aRow][aBestIndex]
              << " lcs_pct=" << aStableLcsPercent[aRow][aBestIndex]
              << " adj_pct=" << aAdjacencyPreservedPercent[aRow][aBestIndex]
              << " retain_pct=" << aPositionRetentionPercent[aRow][aBestIndex]
              << " hist_overlap_pct=" << aHistogramOverlapPercent[aRow][aBestIndex]
              << " exact_match_loops=" << aTotals[aRow][aBestIndex].mExactMatchCount << "/" << aLoops << "\n";
  }

  return 0;
}
