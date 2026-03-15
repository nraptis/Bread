#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/expansion/key_expansion/PasswordExpanderA.hpp"
#include "src/expansion/key_expansion/PasswordExpanderB.hpp"
#include "src/expansion/key_expansion/PasswordExpanderC.hpp"
#include "src/expansion/key_expansion/PasswordExpanderD.hpp"
#include "src/expansion/key_expansion/PasswordExpanderE.hpp"
#include "src/expansion/key_expansion/PasswordExpanderF.hpp"
#include "src/expansion/key_expansion/PasswordExpanderG.hpp"
#include "src/expansion/key_expansion/PasswordExpanderH.hpp"
#include "tests/common/Tests.hpp"

namespace {

std::uint32_t SeedState(int pSalt) {
  const int aSeedSalt = bread::tests::config::ApplyGlobalSeed(pSalt);
  return 0xA24BAED5U ^ (static_cast<std::uint32_t>(aSeedSalt) * 0x9E3779B9U);
}

void FillPassword(std::vector<unsigned char>* pPassword, int pTrial) {
  std::uint32_t aState = SeedState(pTrial + static_cast<int>(pPassword->size()));
  for (std::size_t aIndex = 0; aIndex < pPassword->size(); ++aIndex) {
    aState ^= (aState << 13U);
    aState ^= (aState >> 17U);
    aState ^= (aState << 5U);
    (*pPassword)[aIndex] = static_cast<unsigned char>(aState & 0xFFU);
  }
}

char GradeFromDeviation(double pPercent) {
  if (pPercent <= 25.0) {
    return 'A';
  }
  if (pPercent <= 35.0) {
    return 'B';
  }
  if (pPercent <= 45.0) {
    return 'C';
  }
  if (pPercent <= 55.0) {
    return 'D';
  }
  if (pPercent <= 75.0) {
    return 'E';
  }
  return 'F';
}

struct QualityResult {
  double mExpected = 0.0;
  double mMaxDeviationPct = 0.0;
  int mWorstByte = 0;
  int mWorstCount = 0;
  int mFlaggedOver25Pct = 0;
  char mGrade = 'F';
};

template <typename TExpander>
QualityResult RunQualityForExpander(int pTrials, int pPasswordLength) {
  TExpander aExpander;
  std::array<int, 256> aCounts = {};
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> aExpanded = {};

  for (int aTrial = 0; aTrial < pTrials; ++aTrial) {
    std::vector<unsigned char> aPassword(static_cast<std::size_t>(pPasswordLength), 0U);
    FillPassword(&aPassword, aTrial);
    aExpander.Expand(aPassword.data(), pPasswordLength, aExpanded.data());
    for (unsigned char aByte : aExpanded) {
      ++aCounts[aByte];
    }
  }

  QualityResult aResult;
  const double aTotalBytes = static_cast<double>(pTrials) * static_cast<double>(PASSWORD_EXPANDED_SIZE);
  aResult.mExpected = aTotalBytes / 256.0;
  for (int aByte = 0; aByte < 256; ++aByte) {
    const double aCount = static_cast<double>(aCounts[aByte]);
    const double aDeviation = std::abs(aCount - aResult.mExpected) * 100.0 / aResult.mExpected;
    if (aDeviation > aResult.mMaxDeviationPct) {
      aResult.mMaxDeviationPct = aDeviation;
      aResult.mWorstByte = aByte;
      aResult.mWorstCount = aCounts[aByte];
    }
    if (aCount > (aResult.mExpected * 1.25)) {
      ++aResult.mFlaggedOver25Pct;
    }
  }
  aResult.mGrade = GradeFromDeviation(aResult.mMaxDeviationPct);
  return aResult;
}

template <typename TExpander>
void PrintQualityLine(const char* pName, int pTrials, int pPasswordLength) {
  const QualityResult aResult = RunQualityForExpander<TExpander>(pTrials, pPasswordLength);
  std::cout << "[QUALITY] " << pName
            << " trials=" << pTrials
            << " password_bytes=" << pPasswordLength
            << " expanded_bytes=" << PASSWORD_EXPANDED_SIZE
            << " expected_per_byte=" << static_cast<std::uint64_t>(aResult.mExpected)
            << " max_deviation_pct=" << aResult.mMaxDeviationPct
            << " worst_byte=" << aResult.mWorstByte
            << " worst_count=" << aResult.mWorstCount
            << " bytes_over_25pct=" << aResult.mFlaggedOver25Pct
            << " grade=" << aResult.mGrade << "\n";
}

}  // namespace

int main(int pArgc, char** pArgv) {
  if (!bread::tests::config::BENCHMARK_ENABLED) {
    std::cout << "[SKIP] quality test disabled by Tests.hpp (BENCHMARK_ENABLED=false)\n";
    return 0;
  }

  int aTrials = (bread::tests::config::TEST_LOOP_COUNT <= 0) ? 1 : bread::tests::config::TEST_LOOP_COUNT;
  int aPasswordLength =
      (bread::tests::config::GAME_TEST_DATA_LENGTH <= 0) ? 1 : bread::tests::config::GAME_TEST_DATA_LENGTH;
  if (pArgc >= 2) {
    const int aParsed = std::atoi(pArgv[1]);
    if (aParsed > 0) {
      aTrials = aParsed;
    }
  }
  if (pArgc >= 3) {
    const int aParsed = std::atoi(pArgv[2]);
    if (aParsed > 0) {
      aPasswordLength = aParsed;
    }
  }

  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderA>("PasswordExpanderA", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderB>("PasswordExpanderB", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderC>("PasswordExpanderC", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderD>("PasswordExpanderD", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderE>("PasswordExpanderE", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderF>("PasswordExpanderF", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderG>("PasswordExpanderG", aTrials, aPasswordLength);
  PrintQualityLine<bread::expansion::key_expansion::PasswordExpanderH>("PasswordExpanderH", aTrials, aPasswordLength);

  return 0;
}
