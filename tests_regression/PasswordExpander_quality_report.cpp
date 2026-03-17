#include <array>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "tests/common/PasswordExpanderQualityUtils.hpp"

namespace {

using namespace peanutbutter::tests::password_expander_quality;

template <std::size_t kRows, std::size_t kCols>
void WriteHeatmapTable(std::ofstream& pOut,
                       const char* pTitle,
                       const std::array<const char*, kRows>& pRowLabels,
                       const std::array<const char*, kCols>& pColLabels,
                       const std::array<std::array<double, kCols>, kRows>& pMatrix,
                       bool pUseDiagonal) {
  pOut << "<section>\n";
  pOut << "<h2>" << pTitle << "</h2>\n";
  pOut << "<table>\n<tr><th></th>";
  for (std::size_t aCol = 0; aCol < kCols; ++aCol) {
    pOut << "<th>" << pColLabels[aCol] << "</th>";
  }
  pOut << "</tr>\n";

  for (std::size_t aRow = 0; aRow < kRows; ++aRow) {
    pOut << "<tr><th>" << pRowLabels[aRow] << "</th>";
    for (std::size_t aCol = 0; aCol < kCols; ++aCol) {
      const bool aDiagonal = pUseDiagonal && (aRow == aCol);
      const double aValue = pMatrix[aRow][aCol];
      const std::string aColor = HeatColor(aValue, aDiagonal);
      const char* aTextColor = (aDiagonal || aValue >= 55.0) ? "#ffffff" : "#111827";
      pOut << "<td style=\"background:" << aColor << ";color:" << aTextColor << "\">"
           << std::fixed << std::setprecision(1) << aValue << "</td>";
    }
    pOut << "</tr>\n";
  }

  pOut << "</table>\n</section>\n";
}

template <std::size_t kCount>
void WriteMetricsTable(std::ofstream& pOut,
                       const char* pTitle,
                       const std::array<const char*, kCount>& pLabels,
                       const std::array<StreamMetrics, kCount>& pMetrics) {
  pOut << "<section>\n";
  pOut << "<h2>" << pTitle << "</h2>\n";
  pOut << "<table>\n";
  pOut << "<tr><th>Name</th><th>Predict</th><th>Repeat</th><th>Uniform</th><th>Quality</th>"
          "<th>Entropy</th><th>CondEnt</th><th>Rep64%</th><th>Unique16%</th><th>BlockEq%</th><th>LongestRun</th></tr>\n";
  for (std::size_t aIndex = 0; aIndex < kCount; ++aIndex) {
    const StreamMetrics& aMetric = pMetrics[aIndex];
    pOut << "<tr><th>" << pLabels[aIndex] << "</th>"
         << "<td>" << std::fixed << std::setprecision(2) << aMetric.mPredictabilityScore << "</td>"
         << "<td>" << aMetric.mRepetitivenessScore << "</td>"
         << "<td>" << aMetric.mUniformityScore << "</td>"
         << "<td>" << aMetric.mQualityScore << "</td>"
         << "<td>" << aMetric.mEntropyBits << "</td>"
         << "<td>" << aMetric.mConditionalEntropyBits << "</td>"
         << "<td>" << aMetric.mRepeated64WindowPercent << "</td>"
         << "<td>" << aMetric.mUnique16WindowPercent << "</td>"
         << "<td>" << aMetric.mConsecutiveBlockEqualBytePercent << "</td>"
         << "<td>" << aMetric.mLongestRunLength << "</td></tr>\n";
  }
  pOut << "</table>\n</section>\n";
}

void WriteBestCounterMatches(std::ofstream& pOut,
                             const std::array<std::array<double, kCounterCount>, kTwisterCount>& pMatrix,
                             const std::array<StreamMetrics, kTwisterCount>& pTwisterMetrics,
                             const std::array<StreamMetrics, kCounterCount>& pCounterMetrics) {
  pOut << "<section>\n<h2>Best Counter Match Per Byte Twister</h2>\n";
  for (int aRow = 0; aRow < kTwisterCount; ++aRow) {
    int aBestIndex = 0;
    double aBestScore = pMatrix[aRow][0];
    for (int aCol = 1; aCol < kCounterCount; ++aCol) {
      if (pMatrix[aRow][aCol] > aBestScore) {
        aBestScore = pMatrix[aRow][aCol];
        aBestIndex = aCol;
      }
    }
    pOut << "<div>" << kTwisterLabels[static_cast<std::size_t>(aRow)]
         << " -> " << kCounterLabels[static_cast<std::size_t>(aBestIndex)]
         << " | grade=" << std::fixed << std::setprecision(2) << aBestScore
         << " | twister_quality=" << pTwisterMetrics[static_cast<std::size_t>(aRow)].mQualityScore
         << " | counter_quality=" << pCounterMetrics[static_cast<std::size_t>(aBestIndex)].mQualityScore
         << "</div>\n";
  }
  pOut << "</section>\n";
}

void WriteTwisterNeighbors(std::ofstream& pOut,
                           const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pByteEqualityPercent,
                           const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pBitEqualityPercent,
                           const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pWindowOverlapPercent,
                           const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pHistogramOverlapPercent,
                           const std::array<std::array<PairTotals, kTwisterCount>, kTwisterCount>& pTotals) {
  pOut << "<section>\n<h2>Nearest Neighbors By Byte Equality</h2>\n";
  for (int aRow = 0; aRow < kTwisterCount; ++aRow) {
    int aBestIndex = -1;
    double aBestScore = -1.0;
    for (int aCol = 0; aCol < kTwisterCount; ++aCol) {
      if (aRow == aCol) {
        continue;
      }
      if (pByteEqualityPercent[aRow][aCol] > aBestScore) {
        aBestScore = pByteEqualityPercent[aRow][aCol];
        aBestIndex = aCol;
      }
    }

    pOut << "<div>" << kTwisterLabels[static_cast<std::size_t>(aRow)]
         << " -> " << kTwisterLabels[static_cast<std::size_t>(aBestIndex)]
         << " | byte=" << std::fixed << std::setprecision(2) << aBestScore
         << " | bit=" << pBitEqualityPercent[aRow][aBestIndex]
         << " | window=" << pWindowOverlapPercent[aRow][aBestIndex]
         << " | hist=" << pHistogramOverlapPercent[aRow][aBestIndex]
         << " | exact=" << pTotals[aRow][aBestIndex].mExactMatchCount
         << "</div>\n";
  }
  pOut << "</section>\n";
}

void WriteReport(const std::string& pOutputPath,
                 int pBlockCount,
                 int pDataLength,
                 const std::string& pPassword,
                 const std::array<StreamMetrics, kCounterCount>& pCounterMetrics,
                 const std::array<StreamMetrics, kTwisterCount>& pTwisterMetrics,
                 const std::array<std::array<double, kCounterCount>, kTwisterCount>& pComparisonMatrix,
                 const std::array<std::array<PairTotals, kTwisterCount>, kTwisterCount>& pTotals,
                 const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pByteEqualityPercent,
                 const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pBitEqualityPercent,
                 const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pWindowOverlapPercent,
                 const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pHistogramOverlapPercent) {
  std::ofstream aOut(pOutputPath, std::ios::out | std::ios::trunc);
  if (!aOut.is_open()) {
    throw std::runtime_error("failed to open report output");
  }

  aOut << "<!doctype html>\n<html><head><meta charset=\"utf-8\">";
  aOut << "<title>Password Expander Quality Report</title>";
  aOut << "<style>"
          "body{font-family:Menlo,Consolas,monospace;background:#f8fafc;color:#111827;padding:24px;}"
          "h1,h2{margin:0 0 12px 0;} section{margin-top:24px;}"
          "table{border-collapse:collapse;margin-top:8px;}"
          "th,td{border:1px solid #cbd5e1;padding:6px 8px;text-align:center;font-size:12px;}"
          "th{background:#e2e8f0;} .meta{margin:12px 0 20px 0;} code{background:#e2e8f0;padding:1px 4px;}"
          "</style></head><body>\n";
  aOut << "<h1>Password Expander Quality Report</h1>\n";
  aOut << "<div class=\"meta\">password=<code>" << HtmlEscape(pPassword) << "</code>"
       << " | blocks=" << pBlockCount
       << " | bytes=" << pDataLength
       << " | expanded_size=" << PASSWORD_EXPANDED_SIZE << "</div>\n";
  aOut << "<section><h2>Scoring Notes</h2>\n";
  aOut << "<div>Predictability punishes lag-1 equal-rate drift, lag-1 correlation, and low conditional entropy.</div>\n";
  aOut << "<div>Repetitiveness punishes repeated 64-byte windows, low 16-byte window uniqueness, block-to-block equality, and long runs.</div>\n";
  aOut << "<div>Uniformity punishes reduced-chi2 drift, low entropy, and large byte-frequency deviation.</div>\n";
  aOut << "<div>Quality is the mean of predictability, repetitiveness, and uniformity.</div>\n";
  aOut << "</section>\n";

  WriteMetricsTable(aOut, "Counter Baselines", kCounterLabels, pCounterMetrics);
  WriteMetricsTable(aOut, "Derived Byte Twister Profiles", kTwisterLabels, pTwisterMetrics);
  WriteHeatmapTable(aOut,
                    "Derived Byte Twister vs Counter Profile Grade",
                    kTwisterLabels,
                    kCounterLabels,
                    pComparisonMatrix,
                    false);
  WriteBestCounterMatches(aOut, pComparisonMatrix, pTwisterMetrics, pCounterMetrics);

  WriteHeatmapTable(aOut, "Byte Twister Byte Equality Percent", kTwisterLabels, kTwisterLabels, pByteEqualityPercent, true);
  WriteHeatmapTable(aOut, "Byte Twister Bit Equality Percent", kTwisterLabels, kTwisterLabels, pBitEqualityPercent, true);
  WriteHeatmapTable(aOut, "Byte Twister 16-Byte Window Overlap Percent", kTwisterLabels, kTwisterLabels, pWindowOverlapPercent, true);
  WriteHeatmapTable(aOut, "Byte Twister Histogram Overlap Percent", kTwisterLabels, kTwisterLabels, pHistogramOverlapPercent, true);
  WriteTwisterNeighbors(aOut, pByteEqualityPercent, pBitEqualityPercent, pWindowOverlapPercent, pHistogramOverlapPercent, pTotals);

  aOut << "</body></html>\n";
}

}  // namespace

int main(int pArgc, char** pArgv) {
  const int aBlockCount = (pArgc >= 2 && pArgv[1] != nullptr) ? std::stoi(pArgv[1]) : 2;
  const std::string aPassword = (pArgc >= 3 && pArgv[2] != nullptr) ? pArgv[2] : "hotdog";
  const std::string aOutputPath =
      (pArgc >= 4 && pArgv[3] != nullptr) ? pArgv[3] : "password_expander_quality_report.html";

  int aDataLength = 0;
  if (!TryGetDataLength(aBlockCount, &aDataLength)) {
    std::cerr << "[FAIL] invalid block count: " << aBlockCount << "\n";
    return 1;
  }

  const std::vector<unsigned char> aPasswordBytes = BuildPasswordBytes(aPassword);
  std::array<std::vector<unsigned char>, kCounterCount> aCounterStreams = {};
  std::array<std::vector<unsigned char>, kTwisterCount> aTwisterStreams = {};
  std::array<StreamMetrics, kCounterCount> aCounterMetrics = {};
  std::array<StreamMetrics, kTwisterCount> aTwisterMetrics = {};

  for (int aIndex = 0; aIndex < kCounterCount; ++aIndex) {
    if (!GenerateCounterStream(aIndex, aPasswordBytes, aDataLength, &aCounterStreams[static_cast<std::size_t>(aIndex)])) {
      std::cerr << "[FAIL] failed generating counter stream index=" << aIndex << "\n";
      return 1;
    }
    aCounterMetrics[static_cast<std::size_t>(aIndex)] =
        AnalyzeStream(aCounterStreams[static_cast<std::size_t>(aIndex)]);
  }

  for (int aIndex = 0; aIndex < kTwisterCount; ++aIndex) {
    if (!GenerateTwisterStream(aIndex, aPasswordBytes, aDataLength, &aTwisterStreams[static_cast<std::size_t>(aIndex)])) {
      std::cerr << "[FAIL] failed generating twister stream index=" << aIndex << "\n";
      return 1;
    }
    aTwisterMetrics[static_cast<std::size_t>(aIndex)] =
        AnalyzeStream(aTwisterStreams[static_cast<std::size_t>(aIndex)]);
  }

  std::array<std::array<double, kCounterCount>, kTwisterCount> aComparisonMatrix = {};
  for (int aRow = 0; aRow < kTwisterCount; ++aRow) {
    for (int aCol = 0; aCol < kCounterCount; ++aCol) {
      aComparisonMatrix[aRow][aCol] = CompareMetricsGrade(
          aTwisterMetrics[static_cast<std::size_t>(aRow)],
          aCounterMetrics[static_cast<std::size_t>(aCol)]);
    }
  }

  std::array<std::array<PairTotals, kTwisterCount>, kTwisterCount> aTotals = {};
  std::array<std::array<double, kTwisterCount>, kTwisterCount> aByteEqualityPercent = {};
  std::array<std::array<double, kTwisterCount>, kTwisterCount> aBitEqualityPercent = {};
  std::array<std::array<double, kTwisterCount>, kTwisterCount> aWindowOverlapPercent = {};
  std::array<std::array<double, kTwisterCount>, kTwisterCount> aHistogramOverlapPercent = {};
  ComputeTwisterConfusionMatrices(aTwisterStreams,
                                  1,
                                  &aTotals,
                                  &aByteEqualityPercent,
                                  &aBitEqualityPercent,
                                  &aWindowOverlapPercent,
                                  &aHistogramOverlapPercent);

  try {
    WriteReport(aOutputPath,
                aBlockCount,
                aDataLength,
                aPassword,
                aCounterMetrics,
                aTwisterMetrics,
                aComparisonMatrix,
                aTotals,
                aByteEqualityPercent,
                aBitEqualityPercent,
                aWindowOverlapPercent,
                aHistogramOverlapPercent);
  } catch (const std::exception& pError) {
    std::cerr << "[FAIL] " << pError.what() << "\n";
    return 1;
  }

  std::cout << "[INFO] password expander quality"
            << " password=" << aPassword
            << " blocks=" << aBlockCount
            << " bytes=" << aDataLength << "\n";
  for (int aIndex = 0; aIndex < kTwisterCount; ++aIndex) {
    int aBestCounter = 0;
    double aBestGrade = aComparisonMatrix[aIndex][0];
    for (int aCounter = 1; aCounter < kCounterCount; ++aCounter) {
      if (aComparisonMatrix[aIndex][aCounter] > aBestGrade) {
        aBestGrade = aComparisonMatrix[aIndex][aCounter];
        aBestCounter = aCounter;
      }
    }
    std::cout << "[PAIR] " << kTwisterLabels[static_cast<std::size_t>(aIndex)]
              << " -> " << kCounterLabels[static_cast<std::size_t>(aBestCounter)]
              << " grade=" << std::fixed << std::setprecision(2) << aBestGrade
              << " quality=" << aTwisterMetrics[static_cast<std::size_t>(aIndex)].mQualityScore << "\n";
  }
  std::cout << "[INFO] wrote visual report to " << aOutputPath << "\n";
  return 0;
}
