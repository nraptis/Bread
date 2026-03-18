#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/PeanutButter.hpp"
#include "src/Tables/games/GameBoard.hpp"
#include "src/Tables/maze/MazeDirector.hpp"
#include "src/Tables/password_expanders/PasswordExpander.hpp"

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;

constexpr int kMaterialLength = 64;
constexpr int kHistogramBucketStepPercent = 5;
constexpr int kHistogramBucketCount = (100 / kHistogramBucketStepPercent) + 1;

template <std::size_t kCount>
struct FamilyAggregate {
  std::array<std::array<double, kCount>, kCount> mByteEqualitySum = {};
  std::array<std::uint64_t, kHistogramBucketCount> mByteEqualityHistogram = {};
  std::uint64_t mRepeatChecks = 0U;
  std::uint64_t mRepeatPasses = 0U;
  std::uint64_t mPreservationChecks = 0U;
  std::uint64_t mPreservationPasses = 0U;
  double mHighestByteEqualityPercent = 0.0;
  int mMostSimilarTrialIndex = -1;
  int mMostSimilarLeftIndex = -1;
  int mMostSimilarRightIndex = -1;
  bool mRepeatabilityPassed = true;
  bool mPreservationPassed = true;
};

std::uint32_t MixState(std::uint32_t pState, std::uint32_t pValue) {
  std::uint32_t aState = pState ^ (pValue + 0x9E3779B9U + (pState << 6U) + (pState >> 2U));
  aState ^= (aState << 13U);
  aState ^= (aState >> 17U);
  aState ^= (aState << 5U);
  return aState;
}

std::uint32_t BuildTrialState(int pTrialIndex) {
  return MixState(0xA5C31E29U, static_cast<std::uint32_t>(pTrialIndex + 1));
}

std::string BuildSimplePassword(std::uint32_t* pState) {
  const int aLength = static_cast<int>(MixState(*pState, 0x41U) % 6U);
  *pState = MixState(*pState, static_cast<std::uint32_t>(aLength + 1));

  std::string aPassword;
  aPassword.reserve(static_cast<std::size_t>(aLength));
  for (int aIndex = 0; aIndex < aLength; ++aIndex) {
    *pState = MixState(*pState, static_cast<std::uint32_t>(aIndex + 17));
    aPassword.push_back(static_cast<char>('a' + (*pState % 26U)));
  }
  return aPassword;
}

unsigned char SelectTwisterIndex(std::uint32_t* pState) {
  *pState = MixState(*pState, 0xB7U);
  return static_cast<unsigned char>(*pState % static_cast<std::uint32_t>(PasswordExpander::kTypeCount));
}

void BuildTrialMaterial(int pTrialIndex,
                        const std::string& pPassword,
                        unsigned char pTwisterIndex,
                        std::array<unsigned char, kMaterialLength>* pOutput) {
  std::uint32_t aState = BuildTrialState(pTrialIndex);
  aState = MixState(aState, static_cast<std::uint32_t>(pTwisterIndex + 1U));
  for (char aChar : pPassword) {
    aState = MixState(aState, static_cast<unsigned char>(aChar));
  }

  for (std::size_t aIndex = 0; aIndex < pOutput->size(); ++aIndex) {
    aState = MixState(aState, static_cast<std::uint32_t>((aIndex + 1U) * 29U));
    (*pOutput)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }

  for (std::size_t aIndex = 0; aIndex < pPassword.size(); ++aIndex) {
    const std::size_t aSlot = (aIndex * 11U + static_cast<std::size_t>(pTwisterIndex)) % pOutput->size();
    (*pOutput)[aSlot] ^= static_cast<unsigned char>(pPassword[aIndex]);
  }
}

void ExpandSeed(unsigned char pTwisterIndex,
                const std::array<unsigned char, kMaterialLength>& pMaterial,
                std::size_t pOutputLength,
                std::vector<unsigned char>* pOutput) {
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aWorker = {};
  pOutput->assign(pOutputLength, 0U);
  PasswordExpander::ExpandPasswordBlocksByIndex(pTwisterIndex,
                                                const_cast<unsigned char*>(pMaterial.data()),
                                                static_cast<unsigned int>(pMaterial.size()),
                                                aWorker.data(),
                                                pOutput->data(),
                                                static_cast<unsigned int>(pOutputLength));
}

bool HistogramsMatch(const std::vector<unsigned char>& pLeft, const std::vector<unsigned char>& pRight) {
  if (pLeft.size() != pRight.size()) {
    return false;
  }

  std::array<std::uint32_t, 256> aLeftCounts = {};
  std::array<std::uint32_t, 256> aRightCounts = {};
  for (unsigned char aByte : pLeft) {
    ++aLeftCounts[aByte];
  }
  for (unsigned char aByte : pRight) {
    ++aRightCounts[aByte];
  }
  return aLeftCounts == aRightCounts;
}

double DifferencePercent(const std::vector<unsigned char>& pLeft, const std::vector<unsigned char>& pRight) {
  if (pLeft.size() != pRight.size() || pLeft.empty()) {
    return 0.0;
  }

  std::size_t aDifferent = 0U;
  for (std::size_t aIndex = 0; aIndex < pLeft.size(); ++aIndex) {
    if (pLeft[aIndex] != pRight[aIndex]) {
      ++aDifferent;
    }
  }
  return (100.0 * static_cast<double>(aDifferent)) / static_cast<double>(pLeft.size());
}

bool RunGameOutput(int pGameIndex, const std::vector<unsigned char>& pSeed, std::vector<unsigned char>* pOutput) {
  peanutbutter::games::GameBoard aBoard;
  aBoard.SetGame(pGameIndex);
  aBoard.Seed(const_cast<unsigned char*>(pSeed.data()), static_cast<int>(pSeed.size()));
  pOutput->assign(pSeed.size(), 0U);
  aBoard.Get(pOutput->data(), static_cast<int>(pOutput->size()));
  return true;
}

bool RunMazeOutput(int pMazeIndex, const std::vector<unsigned char>& pSeed, std::vector<unsigned char>* pOutput) {
  peanutbutter::maze::MazeDirector aMaze;
  aMaze.SetGame(pMazeIndex);
  aMaze.Seed(const_cast<unsigned char*>(pSeed.data()), static_cast<int>(pSeed.size()));
  pOutput->assign(pSeed.size(), 0U);
  aMaze.Get(pOutput->data(), static_cast<int>(pOutput->size()));
  return true;
}

const char* GameName(int pGameIndex) {
  peanutbutter::games::GameBoard aBoard;
  aBoard.SetGame(pGameIndex);
  return aBoard.GetName();
}

const char* MazeName(int pMazeIndex) {
  peanutbutter::maze::MazeDirector aMaze;
  aMaze.SetGame(pMazeIndex);
  return aMaze.GetName();
}

int BucketForPercent(double pPercent) {
  const double aClamped = std::max(0.0, std::min(100.0, pPercent));
  int aBucket = static_cast<int>(aClamped / static_cast<double>(kHistogramBucketStepPercent));
  if (aBucket >= kHistogramBucketCount) {
    aBucket = kHistogramBucketCount - 1;
  }
  return aBucket;
}

template <std::size_t kCount>
void UpdateAggregate(FamilyAggregate<kCount>* pAggregate,
                     int pTrialIndex,
                     const std::vector<unsigned char>& pSeed,
                     const std::vector<std::vector<unsigned char>>& pOutputs) {
  for (std::size_t aIndex = 0; aIndex < kCount; ++aIndex) {
    ++pAggregate->mPreservationChecks;
    if (HistogramsMatch(pSeed, pOutputs[aIndex])) {
      ++pAggregate->mPreservationPasses;
    } else {
      pAggregate->mPreservationPassed = false;
    }
  }

  for (std::size_t aLeft = 0; aLeft < kCount; ++aLeft) {
    for (std::size_t aRight = aLeft; aRight < kCount; ++aRight) {
      const double aDifferencePercent =
          (aLeft == aRight) ? 0.0 : DifferencePercent(pOutputs[aLeft], pOutputs[aRight]);
      const double aByteEqualityPercent = 100.0 - aDifferencePercent;
      pAggregate->mByteEqualitySum[aLeft][aRight] += aByteEqualityPercent;
      pAggregate->mByteEqualitySum[aRight][aLeft] += aByteEqualityPercent;

      if (aLeft != aRight) {
        ++pAggregate->mByteEqualityHistogram[BucketForPercent(aByteEqualityPercent)];
        if (aByteEqualityPercent > pAggregate->mHighestByteEqualityPercent) {
          pAggregate->mHighestByteEqualityPercent = aByteEqualityPercent;
          pAggregate->mMostSimilarTrialIndex = pTrialIndex;
          pAggregate->mMostSimilarLeftIndex = static_cast<int>(aLeft);
          pAggregate->mMostSimilarRightIndex = static_cast<int>(aRight);
        }
      }
    }
  }
}

std::string HtmlEscape(const std::string& pText) {
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

std::string SimilarityHeatColor(double pPercent, bool pDiagonal) {
  if (pDiagonal) {
    return "#1f2937";
  }
  const double aClamped = std::max(0.0, std::min(100.0, pPercent));
  const int aRed = static_cast<int>(120.0 + (aClamped * 1.35));
  const int aGreen = static_cast<int>(215.0 - (aClamped * 1.55));
  const int aBlue = static_cast<int>(120.0 - (aClamped * 0.85));
  std::ostringstream aStream;
  aStream << '#'
          << std::hex << std::setfill('0') << std::setw(2) << std::max(0, std::min(255, aRed))
          << std::setw(2) << std::max(0, std::min(255, aGreen))
          << std::setw(2) << std::max(0, std::min(255, aBlue));
  return aStream.str();
}

const char* StatusClass(bool pPassed) {
  return pPassed ? "ok" : "fail";
}

const char* StatusText(bool pPassed) {
  return pPassed ? "GREEN LIGHT" : "RED LIGHT";
}

template <std::size_t kCount>
void WriteSummaryCard(std::ofstream& pOut,
                      const char* pTitle,
                      const FamilyAggregate<kCount>& pAggregate,
                      const std::vector<const char*>& pNames) {
  pOut << "<section class=\"cards\">\n";
  pOut << "<div class=\"card " << StatusClass(pAggregate.mRepeatabilityPassed) << "\">"
       << "<h2>" << pTitle << " Repeatability</h2>"
       << "<div class=\"big\">" << StatusText(pAggregate.mRepeatabilityPassed) << "</div>"
       << "<div>" << pAggregate.mRepeatPasses << " / " << pAggregate.mRepeatChecks << " exact rerun matches</div>"
       << "</div>\n";
  pOut << "<div class=\"card " << StatusClass(pAggregate.mPreservationPassed) << "\">"
       << "<h2>" << pTitle << " Preservation</h2>"
       << "<div class=\"big\">" << StatusText(pAggregate.mPreservationPassed) << "</div>"
       << "<div>" << pAggregate.mPreservationPasses << " / " << pAggregate.mPreservationChecks
       << " seed histogram matches destination histogram</div>"
       << "</div>\n";
  pOut << "<div class=\"card info\">"
       << "<h2>" << pTitle << " Most Similar Pair</h2>"
       << "<div class=\"big\">" << std::fixed << std::setprecision(1) << pAggregate.mHighestByteEqualityPercent
       << "%</div>"
       << "<div>highest bytes-equal match between two different scramblers</div>";
  if (pAggregate.mMostSimilarLeftIndex >= 0 && pAggregate.mMostSimilarRightIndex >= 0) {
    pOut << "<div>trial " << pAggregate.mMostSimilarTrialIndex
         << " : " << HtmlEscape(pNames[static_cast<std::size_t>(pAggregate.mMostSimilarLeftIndex)])
         << " vs " << HtmlEscape(pNames[static_cast<std::size_t>(pAggregate.mMostSimilarRightIndex)]) << "</div>";
  }
  pOut << "</div>\n";
  pOut << "</section>\n";
}

template <std::size_t kCount>
void WriteMatrix(std::ofstream& pOut,
                 const char* pTitle,
                 int pTrialCount,
                 const FamilyAggregate<kCount>& pAggregate,
                 const std::vector<const char*>& pNames) {
  pOut << "<section>\n<h2>" << pTitle << " Byte Equality Matrix (%)</h2>\n";
  pOut << "<table>\n<tr><th></th>";
  for (std::size_t aCol = 0; aCol < kCount; ++aCol) {
    pOut << "<th title=\"" << HtmlEscape(pNames[aCol]) << "\">" << aCol << "</th>";
  }
  pOut << "</tr>\n";

  for (std::size_t aRow = 0; aRow < kCount; ++aRow) {
    pOut << "<tr><th title=\"" << HtmlEscape(pNames[aRow]) << "\">" << aRow << "</th>";
    for (std::size_t aCol = 0; aCol < kCount; ++aCol) {
      const bool aDiagonal = (aRow == aCol);
      const double aAverage = aDiagonal ? 100.0 : (pAggregate.mByteEqualitySum[aRow][aCol] / static_cast<double>(pTrialCount));
      const std::string aColor = SimilarityHeatColor(aAverage, aDiagonal);
      const char* aTextColor = (aDiagonal || aAverage >= 55.0) ? "#ffffff" : "#111827";
      pOut << "<td style=\"background:" << aColor << ";color:" << aTextColor << "\" title=\""
           << HtmlEscape(pNames[aRow]) << " vs " << HtmlEscape(pNames[aCol]) << "\">"
           << std::fixed << std::setprecision(1) << aAverage << "</td>";
    }
    pOut << "</tr>\n";
  }
  pOut << "</table>\n";
  pOut << "<div class=\"legend\">";
  for (std::size_t aIndex = 0; aIndex < kCount; ++aIndex) {
    pOut << "<div>" << aIndex << " = " << HtmlEscape(pNames[aIndex]) << "</div>";
  }
  pOut << "</div>\n";
  pOut << "</section>\n";
}

void WriteHistogramChart(std::ofstream& pOut,
                         const char* pTitle,
                         const std::array<std::uint64_t, kHistogramBucketCount>& pBuckets) {
  std::uint64_t aMaxCount = 1U;
  for (std::uint64_t aCount : pBuckets) {
    aMaxCount = std::max(aMaxCount, aCount);
  }

  pOut << "<section>\n<h2>" << pTitle << " Bytes Equal Histogram</h2>\n";
  pOut << "<div class=\"histogram\">\n";
  for (int aBucket = 0; aBucket < kHistogramBucketCount; ++aBucket) {
    const int aMin = aBucket * kHistogramBucketStepPercent;
    const int aMax = std::min(100, aMin + kHistogramBucketStepPercent);
    const double aHeightPercent = (100.0 * static_cast<double>(pBuckets[static_cast<std::size_t>(aBucket)])) /
                                  static_cast<double>(aMaxCount);
    pOut << "<div class=\"bar-wrap\">"
         << "<div class=\"bar\" style=\"height:" << std::fixed << std::setprecision(1) << aHeightPercent << "%\"></div>"
         << "<div class=\"bar-label\">" << aMin << "-" << aMax << "</div>"
         << "<div class=\"bar-count\">" << pBuckets[static_cast<std::size_t>(aBucket)] << "</div>"
         << "</div>\n";
  }
  pOut << "</div>\n</section>\n";
}

template <std::size_t kCount>
void WriteHtmlReport(const char* pPath,
                     int pTrialCount,
                     const FamilyAggregate<kCount>& pGameAggregate,
                     const FamilyAggregate<kCount>& pMazeAggregate,
                     const std::vector<const char*>& pGameNames,
                     const std::vector<const char*>& pMazeNames) {
  std::ofstream aOut(pPath, std::ios::out | std::ios::trunc);
  if (!aOut.is_open()) {
    return;
  }

  aOut << "<!doctype html>\n<html><head><meta charset=\"utf-8\">";
  aOut << "<title>Scrambler Report</title>";
  aOut << "<style>"
          "body{font-family:Menlo,Consolas,monospace;background:#f7efe3;color:#1f2937;padding:24px;}"
          "h1,h2{margin:0 0 12px 0;} section{margin-top:24px;}"
          ".meta{margin:12px 0 16px 0;}"
          ".note{margin:8px 0 0 0;padding:12px 14px;background:#fff8eb;border:1px solid #d6c6a8;border-radius:12px;}"
          ".cards{display:grid;grid-template-columns:repeat(3,minmax(220px,1fr));gap:16px;}"
          ".card{border-radius:14px;padding:16px;border:1px solid #d6c6a8;background:#fff8eb;}"
          ".card.ok{box-shadow:inset 0 0 0 2px #1f9d55;}"
          ".card.fail{box-shadow:inset 0 0 0 2px #c0392b;}"
          ".card.info{box-shadow:inset 0 0 0 2px #2563eb;}"
          ".big{font-size:28px;font-weight:700;margin:8px 0;}"
          "table{border-collapse:collapse;margin-top:8px;}"
          "th,td{border:1px solid #d7c7ae;padding:6px 8px;text-align:center;font-size:12px;}"
          "th{background:#eadfc8;}"
          ".legend{column-count:2;column-gap:24px;margin-top:12px;}"
          ".histogram{display:flex;align-items:flex-end;gap:6px;height:240px;padding:16px 0;border-bottom:1px solid #d7c7ae;}"
          ".bar-wrap{width:42px;text-align:center;}"
          ".bar{background:linear-gradient(180deg,#c05f3c,#7f1d1d);border-radius:6px 6px 0 0;min-height:2px;}"
          ".bar-label{font-size:10px;margin-top:6px;}"
          ".bar-count{font-size:10px;color:#6b7280;}"
          "</style></head><body>\n";

  aOut << "<h1>Scrambler Family Report</h1>\n";
  aOut << "<div class=\"meta\">trials=" << pTrialCount
       << " | games=" << peanutbutter::games::GameBoard::kGameCount
       << " | mazes=" << peanutbutter::maze::kGameCount << "</div>\n";
  aOut << "<div class=\"note\">Pass or fail is based only on repeatability and seed-histogram preservation. "
       << "The bytes-equal histogram and matrix are visual comparisons across scramblers.</div>\n";

  WriteSummaryCard(aOut, "Games", pGameAggregate, pGameNames);
  WriteSummaryCard(aOut, "Mazes", pMazeAggregate, pMazeNames);
  WriteHistogramChart(aOut, "Games", pGameAggregate.mByteEqualityHistogram);
  WriteHistogramChart(aOut, "Mazes", pMazeAggregate.mByteEqualityHistogram);
  WriteMatrix(aOut, "Games", pTrialCount, pGameAggregate, pGameNames);
  WriteMatrix(aOut, "Mazes", pTrialCount, pMazeAggregate, pMazeNames);

  aOut << "</body></html>\n";
}

}  // namespace

int main(int pArgc, char** pArgv) {
  int aTrialCount = 100;
  if (pArgc >= 2 && pArgv[1] != nullptr) {
    aTrialCount = std::atoi(pArgv[1]);
  }
  if (aTrialCount <= 0) {
    std::cerr << "[FAIL] trial count must be positive\n";
    return 1;
  }

  const char* aReportPath = (pArgc >= 3 && pArgv[2] != nullptr) ? pArgv[2] : "test_all_scramblers_report.html";

  const std::size_t aGameSeedLength = static_cast<std::size_t>(peanutbutter::games::GameBoard::kSeedBufferCapacity);
  const std::size_t aMazeSeedLength = static_cast<std::size_t>(peanutbutter::maze::Maze::kSeedBufferCapacity);
  std::uint64_t aDigest = 1469598103934665603ULL;

  FamilyAggregate<peanutbutter::games::GameBoard::kGameCount> aGameAggregate;
  FamilyAggregate<peanutbutter::maze::kGameCount> aMazeAggregate;

  std::vector<const char*> aGameNames;
  std::vector<const char*> aMazeNames;
  aGameNames.reserve(peanutbutter::games::GameBoard::kGameCount);
  aMazeNames.reserve(peanutbutter::maze::kGameCount);
  for (int aGameIndex = 0; aGameIndex < peanutbutter::games::GameBoard::kGameCount; ++aGameIndex) {
    aGameNames.push_back(GameName(aGameIndex));
  }
  for (int aMazeIndex = 0; aMazeIndex < peanutbutter::maze::kGameCount; ++aMazeIndex) {
    aMazeNames.push_back(MazeName(aMazeIndex));
  }

  for (int aTrialIndex = 0; aTrialIndex < aTrialCount; ++aTrialIndex) {
    std::uint32_t aState = BuildTrialState(aTrialIndex);
    const std::string aPassword = BuildSimplePassword(&aState);
    const unsigned char aTwisterIndex = SelectTwisterIndex(&aState);

    std::array<unsigned char, kMaterialLength> aMaterial = {};
    BuildTrialMaterial(aTrialIndex, aPassword, aTwisterIndex, &aMaterial);

    std::vector<unsigned char> aGameSeed;
    std::vector<unsigned char> aMazeSeed;
    ExpandSeed(aTwisterIndex, aMaterial, aGameSeedLength, &aGameSeed);
    ExpandSeed(aTwisterIndex, aMaterial, aMazeSeedLength, &aMazeSeed);

    std::vector<std::vector<unsigned char>> aGameOutputs(peanutbutter::games::GameBoard::kGameCount);
    std::vector<std::vector<unsigned char>> aMazeOutputs(peanutbutter::maze::kGameCount);

    for (int aGameIndex = 0; aGameIndex < peanutbutter::games::GameBoard::kGameCount; ++aGameIndex) {
      if (!RunGameOutput(aGameIndex, aGameSeed, &aGameOutputs[static_cast<std::size_t>(aGameIndex)])) {
        std::cerr << "[FAIL] game output generation failed trial=" << aTrialIndex << " game=" << aGameIndex << "\n";
        return 1;
      }

      std::vector<unsigned char> aRepeatOutput;
      if (!RunGameOutput(aGameIndex, aGameSeed, &aRepeatOutput)) {
        std::cerr << "[FAIL] game repeat run failed trial=" << aTrialIndex << " game=" << aGameIndex << "\n";
        return 1;
      }
      ++aGameAggregate.mRepeatChecks;
      if (aRepeatOutput == aGameOutputs[static_cast<std::size_t>(aGameIndex)]) {
        ++aGameAggregate.mRepeatPasses;
      } else {
        aGameAggregate.mRepeatabilityPassed = false;
      }
    }

    for (int aMazeIndex = 0; aMazeIndex < peanutbutter::maze::kGameCount; ++aMazeIndex) {
      if (!RunMazeOutput(aMazeIndex, aMazeSeed, &aMazeOutputs[static_cast<std::size_t>(aMazeIndex)])) {
        std::cerr << "[FAIL] maze output generation failed trial=" << aTrialIndex << " maze=" << aMazeIndex << "\n";
        return 1;
      }

      std::vector<unsigned char> aRepeatOutput;
      if (!RunMazeOutput(aMazeIndex, aMazeSeed, &aRepeatOutput)) {
        std::cerr << "[FAIL] maze repeat run failed trial=" << aTrialIndex << " maze=" << aMazeIndex << "\n";
        return 1;
      }
      ++aMazeAggregate.mRepeatChecks;
      if (aRepeatOutput == aMazeOutputs[static_cast<std::size_t>(aMazeIndex)]) {
        ++aMazeAggregate.mRepeatPasses;
      } else {
        aMazeAggregate.mRepeatabilityPassed = false;
      }
    }

    UpdateAggregate(&aGameAggregate, aTrialIndex, aGameSeed, aGameOutputs);
    UpdateAggregate(&aMazeAggregate, aTrialIndex, aMazeSeed, aMazeOutputs);

    for (unsigned char aByte : aGameSeed) {
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
    }
    for (unsigned char aByte : aMazeSeed) {
      aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aByte);
    }
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(aTwisterIndex);
  }

  WriteHtmlReport(aReportPath, aTrialCount, aGameAggregate, aMazeAggregate, aGameNames, aMazeNames);

  const bool aPassed = aGameAggregate.mRepeatabilityPassed && aGameAggregate.mPreservationPassed &&
                       aMazeAggregate.mRepeatabilityPassed && aMazeAggregate.mPreservationPassed;

  if (!aPassed) {
    std::cerr << "[FAIL] all scramblers test failed"
              << " report=" << aReportPath
              << " game_repeat=" << aGameAggregate.mRepeatabilityPassed
              << " game_preservation=" << aGameAggregate.mPreservationPassed
              << " maze_repeat=" << aMazeAggregate.mRepeatabilityPassed
              << " maze_preservation=" << aMazeAggregate.mPreservationPassed << "\n";
    return 1;
  }

  std::cout << "[PASS] all scramblers test passed"
            << " trials=" << aTrialCount
            << " report=" << aReportPath
            << " digest=" << aDigest << "\n";
  return 0;
}
