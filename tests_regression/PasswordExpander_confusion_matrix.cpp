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

void WriteHeatmapTable(std::ofstream& pOut,
                       const char* pTitle,
                       const std::array<std::array<double, kTwisterCount>, kTwisterCount>& pMatrix) {
  pOut << "<section>\n";
  pOut << "<h2>" << pTitle << "</h2>\n";
  pOut << "<table>\n<tr><th></th>";
  for (int aCol = 0; aCol < kTwisterCount; ++aCol) {
    pOut << "<th>" << kTwisterLabels[static_cast<std::size_t>(aCol)] << "</th>";
  }
  pOut << "</tr>\n";

  for (int aRow = 0; aRow < kTwisterCount; ++aRow) {
    pOut << "<tr><th>" << kTwisterLabels[static_cast<std::size_t>(aRow)] << "</th>";
    for (int aCol = 0; aCol < kTwisterCount; ++aCol) {
      const bool aDiagonal = (aRow == aCol);
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

void WriteNearestNeighbors(std::ofstream& pOut,
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
  aOut << "<title>Password Expander Confusion Matrix</title>";
  aOut << "<style>"
          "body{font-family:Menlo,Consolas,monospace;background:#f8fafc;color:#111827;padding:24px;}"
          "h1,h2{margin:0 0 12px 0;} section{margin-top:24px;}"
          "table{border-collapse:collapse;margin-top:8px;}"
          "th,td{border:1px solid #cbd5e1;padding:6px 8px;text-align:center;font-size:12px;}"
          "th{background:#e2e8f0;} .meta{margin:12px 0 20px 0;} code{background:#e2e8f0;padding:1px 4px;}"
          "</style></head><body>\n";
  aOut << "<h1>Password Expander Confusion Matrix</h1>\n";
  aOut << "<div class=\"meta\">password=<code>" << HtmlEscape(pPassword) << "</code>"
       << " | blocks=" << pBlockCount
       << " | bytes=" << pDataLength
       << " | twisters=" << kTwisterCount << "</div>\n";

  WriteHeatmapTable(aOut, "Byte Equality Percent", pByteEqualityPercent);
  WriteHeatmapTable(aOut, "Bit Equality Percent", pBitEqualityPercent);
  WriteHeatmapTable(aOut, "16-Byte Window Overlap Percent", pWindowOverlapPercent);
  WriteHeatmapTable(aOut, "Histogram Overlap Percent", pHistogramOverlapPercent);
  WriteNearestNeighbors(aOut,
                        pByteEqualityPercent,
                        pBitEqualityPercent,
                        pWindowOverlapPercent,
                        pHistogramOverlapPercent,
                        pTotals);
  aOut << "</body></html>\n";
}

}  // namespace

int main(int pArgc, char** pArgv) {
  const int aBlockCount = (pArgc >= 2 && pArgv[1] != nullptr) ? std::stoi(pArgv[1]) : 2;
  const std::string aPassword = (pArgc >= 3 && pArgv[2] != nullptr) ? pArgv[2] : "hotdog";
  const std::string aOutputPath =
      (pArgc >= 4 && pArgv[3] != nullptr) ? pArgv[3] : "password_expander_confusion_matrix.html";

  int aDataLength = 0;
  if (!TryGetDataLength(aBlockCount, &aDataLength)) {
    std::cerr << "[FAIL] invalid block count: " << aBlockCount << "\n";
    return 1;
  }

  const std::vector<unsigned char> aPasswordBytes = BuildPasswordBytes(aPassword);
  std::array<std::vector<unsigned char>, kTwisterCount> aTwisterStreams = {};
  for (int aIndex = 0; aIndex < kTwisterCount; ++aIndex) {
    if (!GenerateTwisterStream(aIndex, aPasswordBytes, aDataLength, &aTwisterStreams[static_cast<std::size_t>(aIndex)])) {
      std::cerr << "[FAIL] failed generating twister stream index=" << aIndex << "\n";
      return 1;
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
                aTotals,
                aByteEqualityPercent,
                aBitEqualityPercent,
                aWindowOverlapPercent,
                aHistogramOverlapPercent);
  } catch (const std::exception& pError) {
    std::cerr << "[FAIL] " << pError.what() << "\n";
    return 1;
  }

  std::cout << "[INFO] password expander confusion matrix"
            << " password=" << aPassword
            << " blocks=" << aBlockCount
            << " bytes=" << aDataLength << "\n";
  std::cout << "[INFO] wrote visual report to " << aOutputPath << "\n";
  return 0;
}
